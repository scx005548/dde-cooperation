#include "Cooperation.h"

#include <stdexcept>
#include <fstream>

#include <sys/stat.h>

#include <uuid/uuid.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "uvxx/Loop.h"
#include "uvxx/TCP.h"
#include "uvxx/UDP.h"
#include "uvxx/Addr.h"
#include "uvxx/Async.h"

#include "Machine.h"
#include "Request.h"
#include "utils/message_helper.h"
#include "protocol/fs.pb.h"

namespace fs = std::filesystem;

const static fs::path inputDevicePath = "/dev/input";
const std::filesystem::path Cooperation::dataDir = "/var/lib/dde-cooperation";

Cooperation::Cooperation()
    : m_uvLoop(uvxx::Loop::defaultLoop())
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_socketScan(std::make_shared<uvxx::UDP>(m_uvLoop))
    , m_listenPair(std::make_shared<uvxx::TCP>(m_uvLoop))
    , m_lastRequestId(0)
    , m_bus(Gio::DBus::Connection::get_sync(Gio::DBus::BusType::BUS_TYPE_SYSTEM))
    , m_service(new DBus::Service{"com.deepin.Cooperation", Gio::DBus::BusType::BUS_TYPE_SYSTEM})
    , m_object(new DBus::Object("/com/deepin/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Cooperation::scan)))
    , m_methodKnock(new DBus::Method("Knock",
                                     DBus::Method::warp(this, &Cooperation::knock),
                                     {{"ip", "s"}, {"port", "i"}}))
    , m_methodRegisterUserDeamon(
          new DBus::Method("RegisterUserDeamon",
                           DBus::Method::warp(this, &Cooperation::registerUserDeamon)))
    , m_propertyMachines(new DBus::Property("Machines",
                                            "ao",
                                            DBus::Property::warp(this, &Cooperation::getMachines)))
    , m_lastMachineIndex(0)
    , m_propertyEnableCooperation(
          new DBus::Property("EnableCooperation",
                             "b",
                             DBus::Property::warp(this, &Cooperation::getEnableCooperation),
                             DBus::Property::warp(this, &Cooperation::setEnableCooperation)))
    , m_enableCooperation(true)
    , m_dbusProxy(Gio::DBus::Proxy::Proxy::create_sync(m_bus,
                                                       "org.freedesktop.DBus",
                                                       "/org/freedesktop/DBus",
                                                       "org.freedesktop.DBus"))
    , m_keypair(dataDir, KeyPair::KeyType::ED25519) {
    ensureDataDirExists();
    initUUID();

    m_keypair.load();

    m_uvThread = std::thread([this]() { m_uvLoop->run(); });

    m_service->registerService();
    m_interface->exportMethod(m_methodScan);
    m_interface->exportMethod(m_methodKnock);
    m_interface->exportMethod(m_methodRegisterUserDeamon);
    m_interface->exportProperty(m_propertyMachines);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    m_socketScan->onSendFailed(uvxx::memFunc(this, &Cooperation::handleSocketError));
    m_socketScan->onReceived(uvxx::memFunc(this, &Cooperation::handleReceivedSocketScan));
    m_socketScan->bind("0.0.0.0", m_scanPort);
    m_socketScan->setBroadcast(true);
    m_socketScan->startRecv();

    m_listenPair->onNewConnection(uvxx::memFunc(this, &Cooperation::handleNewConnection));
    m_listenPair->bind("0.0.0.0");
    m_listenPair->listen();
    m_port = m_listenPair->localAddress()->ipv4()->port();

    std::string ip = Net::getIpAddress();
    m_scanAddr = uvxx::IPv4Addr::create(Net::getBroadcastAddress(ip), m_scanPort);

    // TODO: inotify
    m_async->wake([this]() {
        for (const auto &entry : fs::directory_iterator(inputDevicePath)) {
            if (entry.path().filename().string().rfind("event", 0) == 0) {
                // starts with event
                auto inputDevice = std::make_unique<InputDevice>(entry.path(), m_uvLoop);
                if (inputDevice->shouldIgnore()) {
                    continue;
                }

                inputDevice->init();
                m_inputDevices.insert(std::pair(entry.path(), std::move(inputDevice)));
                spdlog::info("added input device: {}", entry.path().string());
            }
        }
    });

    m_inputEvents.emplace(
        std::make_pair(DeviceType::Keyboard, std::make_unique<InputEvent>(DeviceType::Keyboard)));
    m_inputEvents.emplace(
        std::make_pair(DeviceType::Mouse, std::make_unique<InputEvent>(DeviceType::Mouse)));
    m_inputEvents.emplace(
        std::make_pair(DeviceType::Touchpad, std::make_unique<InputEvent>(DeviceType::Touchpad)));

    m_dbusProxy->signal_signal().connect(
        sigc::mem_fun(this, &Cooperation::handleDBusServiceSignal));
}

Cooperation::~Cooperation() {
    if (m_uvThread.joinable()) {
        m_async->wake([this]() { m_uvLoop->stop(); });

        m_uvThread.join();
    }
}

void Cooperation::ensureDataDirExists() {
    if (fs::exists(dataDir)) {
        if (fs::is_directory(dataDir)) {
            return;
        }

        throw std::runtime_error(fmt::format("{} is not a directory", dataDir.string()));
    }

    auto oldMask = umask(077);
    if (!fs::create_directory(dataDir)) {
        throw std::runtime_error(fmt::format("failed to create directory {}", dataDir.string()));
    }
    umask(oldMask);
}

void Cooperation::initUUID() {
    uuid_t uuid;

    fs::path uuidPath = dataDir / ".uuid";
    if (fs::exists(uuidPath)) {
        std::ifstream f;
        f.open(uuidPath);
        std::getline(f, m_uuid);
        f.close();

        if (uuid_parse(m_uuid.data(), uuid) == 0) {
            return;
        } // else regenerate uuid
    }

    uuid_generate(uuid);
    char uuidStr[100];
    uuid_unparse(uuid, uuidStr);

    m_uuid = uuidStr;
    std::ofstream f;
    f.open(uuidPath);
    f << m_uuid;
    f.close();
}

void Cooperation::scan([[maybe_unused]] const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_userServiceProxy) {
        spdlog::warn("no user service proxy");
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "no user service"});
        return;
    }

    Message msg;
    ScanRequest *request = msg.mutable_scanrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_uuid);
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request->set_port(m_port);

    m_socketScan->send(m_scanAddr, MessageHelper::genMessage(msg));

    m_machines.clear();
    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
    m_lastMachineIndex = 0;

    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::knock(const Glib::VariantContainerBase &args,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_userServiceProxy) {
        spdlog::warn("no user service proxy");
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "no user service"});
        return;
    }

    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<int> port;

    args.get_child(ip, 0);
    args.get_child(port, 1);

    Message msg;
    ScanRequest *request = msg.mutable_scanrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_uuid);
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request->set_port(m_port);

    m_socketScan->send(uvxx::IPv4Addr::create(std::string(ip.get()), port.get()),
                       MessageHelper::genMessage(msg));

    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::registerUserDeamon(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (m_userServiceProxy) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "already registered"});
        return;
    }

    m_userServiceSender = invocation->get_sender();
    invocation->return_value(Glib::VariantContainerBase{});

    m_userServiceProxy = Gio::DBus::Proxy::Proxy::create_sync(m_service->conn(),
                                                              m_userServiceSender,
                                                              "/com/deepin/Cooperation/User",
                                                              "com.deepin.Cooperation.User");
}

std::vector<Glib::DBusObjectPathString> Cooperation::getMachinePaths() const noexcept {
    std::vector<Glib::DBusObjectPathString> machines;
    machines.reserve(m_machines.size());

    std::transform(m_machines.begin(), m_machines.end(), std::back_inserter(machines), [](auto &i) {
        return Glib::DBusObjectPathString(i.second->path());
    });

    return machines;
}

void Cooperation::getMachines(Glib::VariantBase &property,
                              [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths());
}

void Cooperation::getEnableCooperation(
    Glib::VariantBase &property,
    [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<bool>::create(m_enableCooperation);
}

bool Cooperation::setEnableCooperation([[maybe_unused]] const Glib::ustring &propertyName,
                                       const Glib::VariantBase &value) noexcept {
    Glib::Variant<bool> v = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);
    m_enableCooperation = v.get();
    return true;
}

void Cooperation::addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo) {
    auto m = std::make_shared<Machine>(this,
                                       m_uvLoop,
                                       m_service,
                                       m_lastMachineIndex,
                                       ip,
                                       port,
                                       devInfo);
    m_machines.insert(std::pair(devInfo.uuid(), m));
    m_lastMachineIndex++;

    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
}

void Cooperation::handleSocketError(const std::string &title, const std::string &msg) {
    spdlog::error("failed to send message: {} {}", title, msg);
}

void Cooperation::handleReceivedSocketScan(std::shared_ptr<uvxx::Addr> addr,
                                           std::shared_ptr<char[]> data,
                                           [[maybe_unused]] size_t size,
                                           bool partial) noexcept {
    spdlog::debug("partial: {}", partial);

    auto buff = data.get();
    auto header = MessageHelper::parseMessageHeader(buff);
    buff += header_size;
    size -= header_size;

    auto base = MessageHelper::parseMessageBody(buff, header.size);
    spdlog::info("received packet, type: {}", base.payload_case());

    switch (base.payload_case()) {
    case Message::PayloadCase::kScanRequest: {
        // TODO: check ipv4 or ipv6
        auto ipv4 = addr->ipv4();

        const auto &request = base.scanrequest();
        if (request.key() != SCAN_KEY) {
            spdlog::error("key mismatch: {}", request.key());
            return;
        }

        // 自己扫描到自己，忽略
        if (request.deviceinfo().uuid() == m_uuid) {
            return;
        }

        addMachine(ipv4->ip(), request.port(), request.deviceinfo());

        Message msg;
        ScanResponse *response = msg.mutable_scanresponse();
        response->set_key(SCAN_KEY);
        response->mutable_deviceinfo()->set_uuid(m_uuid);
        response->mutable_deviceinfo()->set_name(Net::getHostname());
        response->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
        response->set_port(m_port);

        m_socketScan->send(addr, MessageHelper::genMessage(msg));

        break;
    }
    case Message::PayloadCase::kScanResponse: {
        const auto &resp = base.scanresponse();
        if (resp.key() != SCAN_KEY) {
            spdlog::error("key mismatch: {}", SCAN_KEY);
            return;
        }

        addMachine(addr->ipv4()->ip(), resp.port(), resp.deviceinfo());

        spdlog::info("{} responsed", resp.deviceinfo().name());
        break;
    }
    default: {
        spdlog::error("unknown data type");
        break;
    }
    }
}

void Cooperation::handleNewConnection(bool) noexcept {
    auto socketConnected = m_listenPair->accept();
    socketConnected->onReceived([this, socketConnected](std::unique_ptr<char[]> buffer,
                                                        size_t size) {
        char *buff = buffer.get();
        auto header = MessageHelper::parseMessageHeader(buff);
        buff += header_size;
        size -= header_size;

        Message msg = MessageHelper::parseMessageBody(buff, size);
        buff += header.size;
        size -= header.size;

        const auto &request = msg.pairrequest();
        if (request.key() != SCAN_KEY) {
            spdlog::error("key mismatch {}", SCAN_KEY);
            socketConnected->close();
            return;
        }

        auto i = m_machines.find(request.deviceinfo().uuid());
        if (i == m_machines.end()) {
            // TODO: return failed
            spdlog::error("cannot found device with uuid {}", request.deviceinfo().uuid());
            return;
        }

        auto remote = socketConnected->remoteAddress();
        spdlog::info("connected by {}@{}", request.deviceinfo().name().c_str(), remote->toString());

        i->second->onPair(socketConnected);
    });
    socketConnected->startRead();
}

void Cooperation::handleReceivedCooperateRequest(Machine *machine) {
    spdlog::debug("received cooperate request");

    // TODO: request accept
    if (!m_userServiceProxy) {
        spdlog::warn("no user service proxy");
        return;
    }

    m_lastRequestId++;
    auto r = std::make_shared<Request>(m_service, m_lastRequestId, Request::Type::Cooperation, 0);

    machine->setCooperationRequest(r);

    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<Glib::DBusObjectPathString>::create(r->path()),
    });
    m_userServiceProxy->call_sync("NewRequest", params);
}

void Cooperation::handleStartCooperation(const std::weak_ptr<Machine> &machine) {
    for (auto &inputDevice : m_inputDevices) {
        inputDevice.second->stop();
        inputDevice.second->setMachine(machine);
        inputDevice.second->start();
    }
}

void Cooperation::handleRemoteAcceptedCooperation() {
    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<bool>::create(true),
    });
    m_userServiceProxy->call_sync("StartCooperation", params);
}

void Cooperation::handleStopCooperation() {
    for (auto &inputDevice : m_inputDevices) {
        inputDevice.second->stop();
    }

    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<bool>::create(false),
    });
    m_userServiceProxy->call_sync("StartCooperation", params);
}

bool Cooperation::handleReceivedInputEventRequest(const InputEventRequest &event) {
    auto deviceType = event.devicetype();
    // TODO: allow touchpad
    if (deviceType == DeviceType::Keyboard || deviceType == DeviceType::Mouse) {
        return m_inputEvents[event.devicetype()]->emit(event);
    }

    return false;
}

void Cooperation::handleFlowBack(uint16_t direction, uint16_t x, uint16_t y) {
    for (auto &inputDevice : m_inputDevices) {
        inputDevice.second->stop();
    }

    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<uint16_t>::create(direction),
        Glib::Variant<uint16_t>::create(x),
        Glib::Variant<uint16_t>::create(y),
    });
    m_userServiceProxy->call_sync("FlowBack", params);
}

void Cooperation::handleFlowOut(std::weak_ptr<Machine> machine) {
    for (auto &inputDevice : m_inputDevices) {
        inputDevice.second->stop();
        inputDevice.second->setMachine(machine);
        inputDevice.second->start();
    }
}

void Cooperation::handleDBusServiceSignal([[maybe_unused]] const Glib::ustring &sender,
                                          const Glib::ustring &signal,
                                          const Glib::VariantContainerBase &value) {
    if (!m_userServiceSender.empty()) {
        if (signal == "NameLost") {
            Glib::Variant<Glib::ustring> name;
            value.get_child(name, 0);

            if (name.get() == m_userServiceSender) {
                m_userServiceSender.clear();
                m_userServiceProxy.reset();

                spdlog::info("user service exited");
            }
        }
    }
}
