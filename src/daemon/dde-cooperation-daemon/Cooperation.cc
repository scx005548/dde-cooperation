#include "Cooperation.h"

#include <stdexcept>
#include <fstream>

#include <sys/stat.h>

#include <uuid/uuid.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "Machine.h"
#include "utils/message.h"

namespace fs = std::filesystem;

const static fs::path inputDevicePath = "/dev/input";
const std::filesystem::path Cooperation::dataDir = "/var/lib/dde-cooperation";

Cooperation::Cooperation()
    : m_service(new DBus::Service{"com.deepin.Cooperation", Gio::DBus::BusType::BUS_TYPE_SYSTEM})
    , m_object(new DBus::Object("/com/deepin/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Cooperation::scan)))
    , m_methodKnock(new DBus::Method("Knock",
                                     DBus::Method::warp(this, &Cooperation::knock),
                                     {{"ip", "s"}, {"port", "i"}}))
    , m_propertyMachines(new DBus::Property("Machines",
                                            "ao",
                                            DBus::Property::warp(this, &Cooperation::getMachines)))
    , m_lastMachineIndex(0)
    , m_socketScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                       Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                       Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenPair(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_STREAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_TCP))
    , m_keypair(dataDir, KeyPair::KeyType::ED25519) {
    ensureDataDirExists();
    initUUID();

    m_keypair.load();

    m_service->registerService();
    m_interface->exportMethod(m_methodScan);
    m_interface->exportMethod(m_methodKnock);
    m_interface->exportProperty(m_propertyMachines);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    std::string ip = Net::getIpAddress();
    m_scanAddr = Net::makeSocketAddress(Net::getBroadcastAddress(ip), m_scanPort);

    m_listenScanAddr = Net::makeSocketAddress("0.0.0.0", m_scanPort);
    m_socketListenScan->bind(m_listenScanAddr, true);

    m_listenPairAddr = Net::makeSocketAddress("0.0.0.0", 0);
    m_socketListenPair->bind(m_listenPairAddr, true);
    m_socketListenPair->listen();

    // TODO: inotify
    for (const auto &entry : fs::directory_iterator(inputDevicePath)) {
        if (entry.path().filename().string().rfind("event", 0) == 0) {
            // starts with event
            auto inputDevice = std::make_unique<InputDevice>(entry.path());
            m_inputDevices.insert(std::pair(entry.path(), std::move(inputDevice)));
        }
    }

    m_inputEvents.emplace(
        std::make_pair(DeviceType::Keyboard, std::make_unique<InputEvent>(DeviceType::Keyboard)));
    m_inputEvents.emplace(
        std::make_pair(DeviceType::Mouse, std::make_unique<InputEvent>(DeviceType::Mouse)));

    try {
        Gio::signal_socket().connect(sigc::mem_fun(this, &Cooperation::handleReceivedScanRequest),
                                     m_socketListenScan,
                                     Glib::IO_IN);

        Gio::signal_socket().connect(sigc::mem_fun(this, &Cooperation::handleReceivedScanResponse),
                                     m_socketScan,
                                     Glib::IO_IN);

        Gio::signal_socket().connect(sigc::mem_fun(this, &Cooperation::handleReceivedPairRequest),
                                     m_socketListenPair,
                                     Glib::IO_IN);
    } catch (Gio::Error &e) {
        spdlog::error("{} {}", e.code(), e.what().c_str());
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
    try {
        m_socketScan->set_broadcast(true);
        ScanRequest request;
        request.set_key(SCAN_KEY);
        request.mutable_deviceinfo()->set_uuid(m_uuid);
        request.mutable_deviceinfo()->set_name(Net::getHostname());
        request.mutable_deviceinfo()->set_os(DeviceOS::LINUX);

        auto local = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
            m_socketListenPair->get_local_address());
        request.set_port(local->get_port());

        Message::send_message_to(m_socketScan, MessageType::ScanRequestType, request, m_scanAddr);
        m_machines.clear();
        m_propertyMachines->emitChanged(
            Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
        m_lastMachineIndex = 0;
    } catch (Gio::Error &e) {
        spdlog::error("{} {}", e.code(), e.what().c_str());
    }
    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::knock(const Glib::VariantContainerBase &args,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<int> port;

    args.get_child(ip, 0);
    args.get_child(port, 1);
    auto addr = Net::makeSocketAddress(ip.get(), port.get());
    ScanRequest request;
    request.set_key(SCAN_KEY);
    request.mutable_deviceinfo()->set_uuid(m_uuid);
    request.mutable_deviceinfo()->set_name(Net::getHostname());
    request.mutable_deviceinfo()->set_os(DeviceOS::LINUX);

    auto local = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        m_socketListenPair->get_local_address());
    request.set_port(local->get_port());

    Message::send_message_to(m_socketScan, MessageType::ScanRequestType, request, addr);

    invocation->return_value(Glib::VariantContainerBase{});
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

void Cooperation::addMachine(const Glib::ustring &ip, uint16_t port, const DeviceInfo &devInfo) {
    auto m = std::make_shared<Machine>(*this, m_service, m_lastMachineIndex, ip, port, devInfo);
    m_machines.insert(std::pair(devInfo.uuid(), m));
    m_lastMachineIndex++;

    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
}

bool Cooperation::handleReceivedScanRequest([[maybe_unused]] Glib::IOCondition cond) noexcept {
    Glib::RefPtr<Gio::SocketAddress> addr;
    auto request = Message::recv_message_from<ScanRequest>(m_socketListenScan, addr);
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(addr);
    spdlog::debug("scan request received: {}", std::string(addr->to_string()));

    if (request.key() != SCAN_KEY) {
        spdlog::error("key mismatch {}", SCAN_KEY);
        return true;
    }

    // 自己扫描到自己，忽略
    if (request.deviceinfo().uuid() == m_uuid) {
        return true;
    }

    addMachine(remote->get_address()->to_string(), request.port(), request.deviceinfo());

    ScanResponse response;
    response.set_key(SCAN_KEY);
    response.mutable_deviceinfo()->set_uuid(m_uuid);
    response.mutable_deviceinfo()->set_name(Net::getHostname());
    response.mutable_deviceinfo()->set_os(DeviceOS::LINUX);

    auto local = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        m_socketListenPair->get_local_address());
    response.set_port(local->get_port());

    Message::send_message_to(m_socketListenScan, MessageType::ScanResponseType, response, addr);

    return true;
}

bool Cooperation::handleReceivedScanResponse([[maybe_unused]] Glib::IOCondition cond) noexcept {
    Glib::RefPtr<Gio::SocketAddress> addr;
    auto response = Message::recv_message_from<ScanResponse>(m_socketScan, addr);
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(addr);

    if (response.key() != SCAN_KEY) {
        spdlog::error("key mismatch {}", SCAN_KEY);
        return true;
    }

    addMachine(remote->get_address()->to_string(), response.port(), response.deviceinfo());

    spdlog::info("{} responsed", response.deviceinfo().name());
    return true;
}

bool Cooperation::handleReceivedPairRequest([[maybe_unused]] Glib::IOCondition cond) noexcept {
    auto socketConnected = m_socketListenPair->accept();
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        socketConnected->get_remote_address());
    auto request = Message::recv_message<PairRequest>(socketConnected);
    if (request.key() != SCAN_KEY) {
        spdlog::error("key mismatch {}", SCAN_KEY);
        socketConnected->close();
        return true;
    }

    auto i = m_machines.find(request.deviceinfo().uuid());
    if (i == m_machines.end()) {
        // TODO: return failed
    }

    i->second->onPair(socketConnected);

    spdlog::info("connected by {}@{}:{}\n",
                 request.deviceinfo().name().c_str(),
                 remote->get_address()->to_string().c_str(),
                 remote->get_port());

    return true;
}

bool Cooperation::handleReceivedCooperateRequest(const std::weak_ptr<Machine> &machine) {
    // TODO: request accept

    for (auto &inputDevice : m_inputDevices) {
        inputDevice.second->setMachine(machine);
        inputDevice.second->start();
    }

    return true;
}

bool Cooperation::handleReceivedInputEventRequest(const InputEventRequest &event) {
    return m_inputEvents[event.devicetype()]->emit(event);
}
