#include "Cooperation.h"

#include <spdlog/spdlog.h>

Cooperation::Cooperation() noexcept
    : m_service(
          new DBus::Service{"com.deepin.system.Cooperation", Gio::DBus::BusType::BUS_TYPE_SYSTEM})
    , m_object(new DBus::Object("/com/deepin/system/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.system.Cooperation"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Cooperation::scan)))
    , m_methodPair(new DBus::Method("Pair",
                                    DBus::Method::warp(this, &Cooperation::pair),
                                    {{"ip", "s"}, {"port", "i"}}))
    , m_methodSendFile(new DBus::Method("SendFile",
                                        DBus::Method::warp(this, &Cooperation::sendFile),
                                        {{"filepath", "s"}}))
    , m_propertyDevices(
          new DBus::Property("Devices", "as", DBus::Property::warp(this, &Cooperation::getDevices)))
    , m_propertyPairedDevice(
          new DBus::Property("PairedDevice",
                             "s",
                             DBus::Property::warp(this, &Cooperation::getPairedDevice)))
    , m_socketScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                       Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                       Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenScan(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_DATAGRAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_UDP))
    , m_socketListenPair(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                             Gio::SocketType::SOCKET_TYPE_STREAM,
                                             Gio::SocketProtocol::SOCKET_PROTOCOL_TCP))
    , m_socketConnect(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                          Gio::SocketType::SOCKET_TYPE_STREAM,
                                          Gio::SocketProtocol::SOCKET_PROTOCOL_TCP))
    , m_socketConnected(Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                            Gio::SocketType::SOCKET_TYPE_STREAM,
                                            Gio::SocketProtocol::SOCKET_PROTOCOL_TCP)) {
    m_service->registerService();
    m_interface->exportMethod(m_methodScan);
    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportProperty(m_propertyDevices);
    m_interface->exportProperty(m_propertyPairedDevice);
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

void Cooperation::scan([[maybe_unused]] const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    try {
        m_socketScan->set_broadcast(true);
        ScanRequest request;
        request.set_key(SCAN_KEY);
        request.set_master_name(Net::getHostname());
        request.set_os(DeviceOS::LINUX);
        Message::send_message_to(m_socketScan, MessageType::ScanRequestType, request, m_scanAddr);
        m_devices.clear();
    } catch (Gio::Error &e) {
        SPDLOG_ERROR("{} {}", e.code(), e.what().c_str());
    }
    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::pair(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<int> port;

    args.get_child(ip, 0);
    args.get_child(port, 1);
    m_socketConnect->connect(Net::makeSocketAddress(ip.get(), port.get()));
    Gio::signal_socket().connect(
        [this](Glib::IOCondition cond) { return m_mainHandler(cond, m_socketConnect); },
        m_socketConnect,
        Glib::IO_IN);

    m_fileSender.setPilot(m_socketConnect);

    PairRequest request;
    request.set_key(SCAN_KEY);
    request.set_master_name(Net::getHostname());
    request.set_os(DeviceOS::LINUX);
    Message::send_message(m_socketConnect, MessageType::PairRequestType, request);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::sendFile(const Glib::VariantContainerBase &args,
                           const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> filepath;
    args.get_child(filepath, 0);
    auto request = m_fileSender.makeStopTransferRequest();
    m_fileSender.pushFile(filepath.get());
    Message::send_message(m_socketConnect, TransferRequestType, request);
    invocation->return_value(Glib::VariantContainerBase{});
}

void Cooperation::getDevices(Glib::VariantBase &property,
                             [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    auto devices = Glib::Variant<std::vector<Glib::ustring>>::create(m_devices);
    property = devices;
}

void Cooperation::getPairedDevice(Glib::VariantBase &property,
                                  [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    auto device = Glib::Variant<Glib::ustring>::create(m_pairedDevice);
    property = device;
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
    response.set_slave_name(Net::getHostname());
    response.set_os(DeviceOS::LINUX);

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

    auto device = Glib::ustring::compose("%1@%2:%3",
                                         response.slave_name(),
                                         remote->get_address()->to_string(),
                                         response.port());
    m_devices.push_back(device);
    SPDLOG_INFO("{} responsed", device.c_str());
    return true;
}

bool Cooperation::m_pairRequestHandler([[maybe_unused]] Glib::IOCondition cond) noexcept {

    m_socketConnected = m_socketListenPair->accept();
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        m_socketConnected->get_remote_address());
    auto request = Message::recv_message<PairRequest>(m_socketConnected);
    if (request.key() != SCAN_KEY) {
        SPDLOG_ERROR("key mismatch {}", SCAN_KEY);
        m_socketConnected->close();
        return true;
    }

    SPDLOG_INFO("connected by {}@{}:{}\n",
                request.master_name().c_str(),
                remote->get_address()->to_string().c_str(),
                remote->get_port());
    Gio::signal_socket().connect(
        [this](Glib::IOCondition cond) { return m_mainHandler(cond, m_socketConnected); },
        m_socketConnected,
        Glib::IO_IN);

    PairResponse response;
    response.set_key(SCAN_KEY);
    response.set_slave_name(Net::getHostname());
    response.set_os(DeviceOS::LINUX);
    response.set_agree(true); // TODO: 询问用户是否同意
    Message::send_message(m_socketConnected, MessageType::PairResponseType, response);

    return true;
}

bool Cooperation::m_mainHandler([[maybe_unused]] Glib::IOCondition cond,
                                const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    auto base = Message::recv_message_header(sock);
    switch (base.type()) {
    case TransferRequestType: {
        auto response = m_fileReciever.parseRequest(
            Message::recv_message_body<TransferRequest>(sock, base));
        Message::send_message<TransferResponse>(sock, TransferResponseType, response);
        break;
    }

    case TransferResponseType:
        m_fileSender.parseResponse(Message::recv_message_body<TransferResponse>(sock, base));
        break;

    default:
        break;
    }

    return true;
}
