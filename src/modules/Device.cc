#include "Device.h"

#include "utils/net.h"
#include "utils/message.h"

#include "protocol/pair.pb.h"

Device::Device(Glib::RefPtr<DBus::Service> service, uint32_t id, const DeviceInfo &sp)
    : m_path(Glib::ustring::compose("/com/deepin/Cooperation/Device/%1", id))
    , m_service(service)
    , m_object(new DBus::Object(m_path))
    , m_interface(new DBus::Interface("com.deepin.Cooperation.Device"))
    , m_methodPair(new DBus::Method("Pair", DBus::Method::warp(this, &Device::pair)))
    , m_methodSendFile(new DBus::Method("SendFile",
                                        DBus::Method::warp(this, &Device::sendFile),
                                        {{"filepath", "s"}}))
    , m_name(sp.name())
    , m_propertyName(new DBus::Property("Name", "s", DBus::Property::warp(this, &Device::getName)))
    , m_paired(false)
    , m_propertyPaired(
          new DBus::Property("Paired", "b", DBus::Property::warp(this, &Device::getPaired)))
    , m_os(sp.os())
    , m_propertyOS(new DBus::Property("OS", "u", DBus::Property::warp(this, &Device::getOS)))
    , m_compositor(sp.compositor())
    , m_propertyCompositor(new DBus::Property("Compositor",
                                              "u",
                                              DBus::Property::warp(this, &Device::getCompositor))) {

    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportProperty(m_propertyName);
    m_interface->exportProperty(m_propertyPaired);
    m_interface->exportProperty(m_propertyOS);
    m_interface->exportProperty(m_propertyCompositor);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);
}

Device::~Device() {
    m_service->unexportObject(m_object->path());
}

void Device::pair(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<int> port;

    m_socketConnect = Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                          Gio::SocketType::SOCKET_TYPE_STREAM,
                                          Gio::SocketProtocol::SOCKET_PROTOCOL_TCP);

    args.get_child(ip, 0);
    args.get_child(port, 1);
    m_socketConnect->connect(Net::makeSocketAddress(ip.get(), port.get()));
    Gio::signal_socket().connect(
        [this](Glib::IOCondition cond) { return mainHandler(cond, m_socketConnect); },
        m_socketConnect,
        Glib::IO_IN);

    m_fileSender.setPilot(m_socketConnect);

    PairRequest request;
    request.set_key(SCAN_KEY);
    request.mutable_deviceinfo()->set_name(Net::getHostname());
    request.mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request.mutable_deviceinfo()->set_compositor(Compositor::NONE);
    Message::send_message(m_socketConnect, MessageType::PairRequestType, request);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Device::onPair(Glib::RefPtr<Gio::Socket> conn) {
    m_socketConnect = conn;
    m_paired = true;

    PairResponse response;
    response.set_key(SCAN_KEY);
    response.mutable_deviceinfo()->set_name(Net::getHostname());
    response.mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    response.mutable_deviceinfo()->set_compositor(Compositor::NONE);
    response.set_agree(true); // TODO: 询问用户是否同意
    Message::send_message(m_socketConnect, MessageType::PairResponseType, response);
}

void Device::sendFile(const Glib::VariantContainerBase &args,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> filepath;
    args.get_child(filepath, 0);
    auto request = m_fileSender.makeStopTransferRequest();
    m_fileSender.pushFile(filepath.get());
    Message::send_message(m_socketConnect, TransferRequestType, request);
    invocation->return_value(Glib::VariantContainerBase{});
}

void Device::getName(Glib::VariantBase &property,
                     [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<Glib::ustring>::create(m_name);
}

void Device::getPaired(Glib::VariantBase &property,
                       [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<bool>::create(m_paired);
}

void Device::getOS(Glib::VariantBase &property,
                   [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint32_t>::create(m_os);
}

void Device::getCompositor(Glib::VariantBase &property,
                           [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint32_t>::create(m_compositor);
}

bool Device::mainHandler([[maybe_unused]] Glib::IOCondition cond,
                           const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    auto base = Message::recv_message_header(sock);
    switch (base.type()) {
    case PairResponseType: {
        handlePairRespinse(Message::recv_message_body<PairResponse>(sock, base));
        break;
    }

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

void Device::handlePairRespinse(const PairResponse &resp) {
    bool agree = resp.agree();
    if (agree) {
        m_paired = true;
        return;
    }

    // TODO: handle not agree
}
