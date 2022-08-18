#include "Manager.h"

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
#include "uvxx/Process.h"
#include "uvxx/Pipe.h"
#include "uvxx/Pipe.h"

#include "Machine.h"
#include "Request.h"
#include "X11/Display.h"
#include "X11/Clipboard.h"
#include "utils/message_helper.h"
#include "protocol/message.pb.h"
#include "protocol/ipc_message.pb.h"

namespace fs = std::filesystem;

const static fs::path inputDevicePath = "/dev/input";

Manager::Manager(const std::shared_ptr<uvxx::Loop> &uvLoop, const std::filesystem::path &dataDir)
    : m_dataDir(dataDir)
    , m_mountRoot(m_dataDir / "mr")
    , m_lastMachineIndex(0)
    , m_enableCooperation(true)
    , m_lastRequestId(0)
    , m_uvLoop(uvLoop)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_socketScan(std::make_shared<uvxx::UDP>(m_uvLoop))
    , m_listenPair(std::make_shared<uvxx::TCP>(m_uvLoop))
    , m_bus(Gio::DBus::Connection::get_sync(Gio::DBus::BusType::BUS_TYPE_SESSION))
    , m_service(new DBus::Service{"com.deepin.Cooperation", Gio::DBus::BusType::BUS_TYPE_SESSION})
    , m_object(new DBus::Object("/com/deepin/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Manager::scan)))
    , m_methodKnock(new DBus::Method("Knock",
                                     DBus::Method::warp(this, &Manager::knock),
                                     {{"ip", "s"}, {"port", "i"}}))
    , m_propertyMachines(
          new DBus::Property("Machines", "ao", DBus::Property::warp(this, &Manager::getMachines)))
    , m_propertyEnableCooperation(
          new DBus::Property("EnableCooperation",
                             "b",
                             DBus::Property::warp(this, &Manager::getEnableCooperation),
                             DBus::Property::warp(this, &Manager::setEnableCooperation)))
    , m_keypair(m_dataDir, KeyPair::KeyType::ED25519) {
    ensureDataDirExists();
    initUUID();

    m_keypair.load();

    m_service->registerService();
    m_interface->exportMethod(m_methodScan);
    m_interface->exportMethod(m_methodKnock);
    m_interface->exportProperty(m_propertyMachines);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    m_socketScan->onSendFailed(uvxx::memFunc(this, &Manager::handleSocketError));
    m_socketScan->onReceived(uvxx::memFunc(this, &Manager::handleReceivedSocketScan));
    m_socketScan->bind("0.0.0.0", m_scanPort);
    m_socketScan->setBroadcast(true);
    m_socketScan->startRecv();

    m_listenPair->onNewConnection(uvxx::memFunc(this, &Manager::handleNewConnection));
    m_listenPair->bind("0.0.0.0");
    m_listenPair->listen();
    m_port = m_listenPair->localAddress()->ipv4()->port();

    std::string ip = Net::getIpAddress();
    m_scanAddr = uvxx::IPv4Addr::create(Net::getBroadcastAddress(ip), m_scanPort);

    m_displayServer = std::make_unique<X11::Display>(m_uvLoop, this);
    m_clipboard = std::make_unique<X11::Clipboard>(m_uvLoop, this);

    // TODO: inotify
    for (const auto &entry : fs::directory_iterator(inputDevicePath)) {
        if (!entry.is_character_file()) {
            continue;
        }
        if (entry.path().filename().string().rfind("event", 0) != 0) {
            continue;
        }

        m_inputGrabbers.emplace(
            std::make_pair(entry.path().string(),
                           std::make_shared<InputGrabberWrapper>(this, m_uvLoop, entry.path())));
    }
}

Manager::~Manager() {
}

void Manager::ensureDataDirExists() {
    if (fs::exists(m_dataDir)) {
        if (fs::is_directory(m_dataDir)) {
            return;
        }

        throw std::runtime_error(fmt::format("{} is not a directory", m_dataDir.string()));
    }

    auto oldMask = umask(077);
    if (!fs::create_directory(m_dataDir)) {
        throw std::runtime_error(fmt::format("failed to create directory {}", m_dataDir.string()));
    }
    umask(oldMask);
}

void Manager::initUUID() {
    uuid_t uuid;

    fs::path uuidPath = m_dataDir / ".uuid";
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

void Manager::scan([[maybe_unused]] const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Message base;
    ScanRequest *request = base.mutable_scanrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_uuid);
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request->set_port(m_port);

    m_socketScan->send(m_scanAddr, MessageHelper::genMessage(base));

    m_machines.clear();
    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
    m_lastMachineIndex = 0;

    invocation->return_value(Glib::VariantContainerBase{});
}

void Manager::knock(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
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

std::vector<Glib::DBusObjectPathString> Manager::getMachinePaths() const noexcept {
    std::vector<Glib::DBusObjectPathString> machines;
    machines.reserve(m_machines.size());

    std::transform(m_machines.begin(), m_machines.end(), std::back_inserter(machines), [](auto &i) {
        return Glib::DBusObjectPathString(i.second->path());
    });

    return machines;
}

void Manager::getMachines(Glib::VariantBase &property,
                          [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths());
}

void Manager::getEnableCooperation(
    Glib::VariantBase &property,
    [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<bool>::create(m_enableCooperation);
}

bool Manager::setEnableCooperation([[maybe_unused]] const Glib::ustring &propertyName,
                                   const Glib::VariantBase &value) noexcept {
    Glib::Variant<bool> v = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);
    m_enableCooperation = v.get();
    return true;
}

void Manager::removeInputGrabber(const std::filesystem::path &path) {
    m_inputGrabbers.erase(path.string());
}

bool Manager::tryFlowOut(uint16_t direction, uint16_t x, uint16_t y) {
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;
        if (machine->m_direction == direction) {
            machine->flowTo(direction, x, y);
            onFlowOut(machine);
            return true;
        }
    }

    return false;
}

void Manager::addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo) {
    auto shortUUID = devInfo.uuid();
    shortUUID.resize(8);
    fs::path dataPath = m_dataDir / shortUUID;
    auto m = std::make_shared<Machine>(this,
                                       m_clipboard.get(),
                                       m_uvLoop,
                                       m_service,
                                       m_lastMachineIndex,
                                       dataPath,
                                       ip,
                                       port,
                                       devInfo);
    m_machines.insert(std::pair(devInfo.uuid(), m));
    m_lastMachineIndex++;

    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
}

void Manager::handleSocketError(const std::string &title, const std::string &msg) {
    spdlog::error("failed to send message: {} {}", title, msg);
}

void Manager::handleReceivedSocketScan(std::shared_ptr<uvxx::Addr> addr,
                                       std::shared_ptr<char[]> data,
                                       [[maybe_unused]] size_t size,
                                       bool partial) noexcept {
    spdlog::debug("partial: {}", partial);

    auto buff = data.get();
    auto header = MessageHelper::parseMessageHeader(buff);
    buff += header_size;
    size -= header_size;

    auto base = MessageHelper::parseMessageBody<Message>(buff, header.size);
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

        spdlog::info("{} responded", resp.deviceinfo().name());
        break;
    }
    default: {
        spdlog::error("unknown data type");
        break;
    }
    }
}

void Manager::handleNewConnection(bool) noexcept {
    auto socketConnected = m_listenPair->accept();
    socketConnected->onReceived([this, socketConnected](uvxx::Buffer &buff) {
        auto res = MessageHelper::parseMessage<Message>(buff);
        if (!res.has_value()) {
            return;
        }

        Message &msg = res.value();

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

void Manager::onStartCooperation() {
    m_displayServer->startEdgeDetection();
}

void Manager::onStopCooperation() {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber.second->stop();
    }

    m_displayServer->stopEdgeDetection();
}

void Manager::onFlowBack(uint16_t direction, uint16_t x, uint16_t y) {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber.second->stop();
    }

    m_displayServer->flowBack(direction, x, y);
}

void Manager::onFlowOut(std::weak_ptr<Machine> machine) {
    for (auto &inputGrabbers : m_inputGrabbers) {
        inputGrabbers.second->setMachine(machine);
        inputGrabbers.second->start();
    }
}

void Manager::onClipboardTargetsChanged(const std::vector<std::string> &targets) {
    for (auto &[uuid, machine] : m_machines) {
        if (!machine->m_cooperating) {
            continue;
        }

        machine->onClipboardTargetsChanged(targets);
    }
}

void Manager::onMachineOwnClipboard(const std::weak_ptr<Machine> &machine,
                                    const std::vector<std::string> &targets) {
    m_clipboard->newClipboardOwnerTargets(machine, targets);
}
