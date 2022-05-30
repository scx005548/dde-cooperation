#include "Cooperation.h"

#include <stdexcept>
#include <fstream>

#include <sys/stat.h>

#include <uuid/uuid.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

const std::filesystem::path Cooperation::dataDir = "/var/lib/dde-cooperation";

Cooperation::Cooperation()
    : m_service(new DBus::Service{"com.deepin.Cooperation", Gio::DBus::BusType::BUS_TYPE_SYSTEM})
    , m_object(new DBus::Object("/com/deepin/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Cooperation::scan)))
    , m_propertyDevices(
          new DBus::Property("Devices", "ao", DBus::Property::warp(this, &Cooperation::getDevices)))
    , m_lastDeviceIndex(0)
    , m_socketScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                       Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                       Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenPair(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_STREAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_TCP)) {
    ensureDataDirExists();
    initUUID();

    m_service->registerService();
    m_interface->exportMethod(m_methodScan);
    m_interface->exportProperty(m_propertyDevices);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    std::string ip = Net::getIpAddress();
    m_scanAddr = Net::makeSocketAddress(Net::getBroadcastAddress(ip), m_scanPort);

    m_listenScanAddr = Net::makeSocketAddress("0.0.0.0", m_scanPort);
    m_socketListenScan->bind(m_listenScanAddr, true);

    m_listenPairAddr = Net::makeSocketAddress("0.0.0.0", 0);
    m_socketListenPair->bind(m_listenPairAddr, true);
    m_socketListenPair->listen();

    try {
        Gio::signal_socket().connect(
            [this](Glib::IOCondition cond) { return m_scanRequestHandler(cond); },
            m_socketListenScan,
            Glib::IO_IN);

        Gio::signal_socket().connect(
            [this](Glib::IOCondition cond) { return m_scanResponseHandler(cond); },
            m_socketScan,
            Glib::IO_IN);

        Gio::signal_socket().connect(
            [this](Glib::IOCondition cond) { return m_pairRequestHandler(cond); },
            m_socketListenPair,
            Glib::IO_IN);
    } catch (Gio::Error &e) {
        SPDLOG_ERROR("{} {}", e.code(), e.what().c_str());
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
        Message::send_message_to(m_socketScan, MessageType::ScanRequestType, request, m_scanAddr);
        m_devices.clear();
        m_lastDeviceIndex = 0;
    } catch (Gio::Error &e) {
        SPDLOG_ERROR("{} {}", e.code(), e.what().c_str());
    }
    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::getDevices(Glib::VariantBase &property,
                             [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    std::vector<Glib::ustring> devices;
    devices.reserve(m_devices.size());

    std::transform(m_devices.begin(), m_devices.end(), std::back_inserter(devices), [](auto &i) {
        return i.second.path();
    });

    property = Glib::Variant<std::vector<Glib::ustring>>::create(devices);
}

bool Cooperation::m_scanRequestHandler([[maybe_unused]] Glib::IOCondition cond) const noexcept {
    Glib::RefPtr<Gio::SocketAddress> addr;
    auto request = Message::recv_message_from<ScanRequest>(m_socketListenScan, addr);

    if (request.key() != SCAN_KEY) {
        SPDLOG_ERROR("key mismatch {}", SCAN_KEY);
        return true;
    }

    // 自己扫描到自己，忽略
    // auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(addr);
    // if (remote->get_address()->to_string() == Net::getIpAddress())
    // {
    //     return true;
    // }

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

bool Cooperation::m_scanResponseHandler([[maybe_unused]] Glib::IOCondition cond) noexcept {
    Glib::RefPtr<Gio::SocketAddress> addr;
    auto response = Message::recv_message_from<ScanResponse>(m_socketScan, addr);
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(addr);

    if (response.key() != SCAN_KEY) {
        SPDLOG_ERROR("key mismatch {}", SCAN_KEY);
        return true;
    }

    m_devices.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(response.deviceinfo().uuid()),
        std::forward_as_tuple(*this, m_service, m_lastDeviceIndex, response.deviceinfo()));
    m_lastDeviceIndex++;
    SPDLOG_INFO("{} responsed", response.deviceinfo().name());
    return true;
}

bool Cooperation::m_pairRequestHandler([[maybe_unused]] Glib::IOCondition cond) noexcept {
    auto socketConnected = m_socketListenPair->accept();
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        socketConnected->get_remote_address());
    auto request = Message::recv_message<PairRequest>(socketConnected);
    if (request.key() != SCAN_KEY) {
        SPDLOG_ERROR("key mismatch {}", SCAN_KEY);
        socketConnected->close();
        return true;
    }

    Device &device = ([this, &request]() -> Device & {
        auto i = m_devices.find(request.deviceinfo().uuid());
        if (i == m_devices.end()) {
            return std::get<0>(m_devices.emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(request.deviceinfo().uuid()),
                                                 std::forward_as_tuple(*this,
                                                                       m_service,
                                                                       m_lastDeviceIndex,
                                                                       request.deviceinfo())))
                ->second;
        }

        return i->second;
    })();

    device.onPair(socketConnected);

    SPDLOG_INFO("connected by {}@{}:{}\n",
                request.deviceinfo().name().c_str(),
                remote->get_address()->to_string().c_str(),
                remote->get_port());

    return true;
}
