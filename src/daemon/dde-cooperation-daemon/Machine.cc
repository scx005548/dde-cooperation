#include "Machine.h"

#include "Cooperation.h"
#include "utils/net.h"
#include "utils/message.h"

#include "protocol/pair.pb.h"
#include "protocol/cooperation.pb.h"

Machine::Machine(Cooperation &cooperation,
                 Glib::RefPtr<DBus::Service> service,
                 uint32_t id,
                 const DeviceInfo &sp)
    : m_cooperation(cooperation)
    , m_path(Glib::ustring::compose("/com/deepin/Cooperation/Machine/%1", id))
    , m_service(service)
    , m_object(new DBus::Object(m_path))
    , m_interface(new DBus::Interface("com.deepin.Cooperation.Machine"))
    , m_methodPair(new DBus::Method("Pair", DBus::Method::warp(this, &Machine::pair)))
    , m_methodSendFile(new DBus::Method("SendFile",
                                        DBus::Method::warp(this, &Machine::sendFile),
                                        {{"filepath", "s"}}))
    , m_uuid(sp.uuid())
    , m_propertyUUID(new DBus::Property("UUID", "s", DBus::Property::warp(this, &Machine::getUUID)))
    , m_name(sp.name())
    , m_propertyName(new DBus::Property("Name", "s", DBus::Property::warp(this, &Machine::getName)))
    , m_paired(false)
    , m_propertyPaired(
          new DBus::Property("Paired", "b", DBus::Property::warp(this, &Machine::getPaired)))
    , m_os(sp.os())
    , m_propertyOS(new DBus::Property("OS", "u", DBus::Property::warp(this, &Machine::getOS)))
    , m_compositor(sp.compositor())
    , m_propertyCompositor(
          new DBus::Property("Compositor",
                             "u",
                             DBus::Property::warp(this, &Machine::getCompositor))) {

    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportProperty(m_propertyName);
    m_interface->exportProperty(m_propertyPaired);
    m_interface->exportProperty(m_propertyOS);
    m_interface->exportProperty(m_propertyCompositor);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);
}

Machine::~Machine() {
    m_service->unexportObject(m_object->path());
}

void Machine::pair(const Glib::VariantContainerBase &args,
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

    PairRequest request;
    request.set_key(SCAN_KEY);
    request.mutable_deviceinfo()->set_uuid(m_cooperation.uuid());
    request.mutable_deviceinfo()->set_name(Net::getHostname());
    request.mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request.mutable_deviceinfo()->set_compositor(Compositor::NONE);
    Message::send_message(m_socketConnect, MessageType::PairRequestType, request);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::onPair(Glib::RefPtr<Gio::Socket> conn) {
    m_socketConnect = conn;
    m_paired = true;

    PairResponse response;
    response.set_key(SCAN_KEY);
    response.mutable_deviceinfo()->set_uuid(m_cooperation.uuid());
    response.mutable_deviceinfo()->set_name(Net::getHostname());
    response.mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    response.mutable_deviceinfo()->set_compositor(Compositor::NONE);
    response.set_agree(true); // TODO: 询问用户是否同意
    Message::send_message(m_socketConnect, MessageType::PairResponseType, response);
}

void Machine::sendFile(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> filepath;
    args.get_child(filepath, 0);

    // TODO: impl

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::getUUID(Glib::VariantBase &property,
                      [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<Glib::ustring>::create(m_uuid);
}

void Machine::getName(Glib::VariantBase &property,
                      [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<Glib::ustring>::create(m_name);
}

void Machine::getPaired(Glib::VariantBase &property,
                        [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<bool>::create(m_paired);
}

void Machine::getOS(Glib::VariantBase &property,
                    [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint32_t>::create(m_os);
}

void Machine::getCompositor(Glib::VariantBase &property,
                            [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint32_t>::create(m_compositor);
}

bool Machine::mainHandler([[maybe_unused]] Glib::IOCondition cond,
                          const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    auto base = Message::recv_message_header(sock);
    switch (base.type()) {
    case PairResponseType: {
        handlePairResponse(Message::recv_message_body<PairResponse>(sock, base));
        break;
    }

    case InputEventRequestType: {
        auto event = Message::recv_message_body<InputEventRequest>(sock, base);
        m_signal_inputEvent.emit(event);

        InputEventResponse response;
        response.set_serial(event.serial());
        response.set_success(true);
        Message::send_message(sock, InputEventResponseType, response);
        break;
    }

    case InputEventResponseType: {
        break;
    }

    default:
        break;
    }

    return true;
}

void Machine::handlePairResponse(const PairResponse &resp) {
    bool agree = resp.agree();
    if (agree) {
        m_paired = true;
        return;
    }

    // TODO: handle not agree
}

void Machine::handleCooperateRequest() {
    bool accept = m_signal_cooperationRequest.emit(this);

    CooperateResponse resp;
    resp.set_accept(accept);
    Message::send_message(m_socketConnect, CooperateResponseType, resp);
}

void Machine::handleInputEvent(const InputEventRequest &event) {
    Message::send_message(m_socketConnect, InputEventRequestType, event);
}
