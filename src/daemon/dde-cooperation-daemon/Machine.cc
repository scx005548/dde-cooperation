#include "Machine.h"

#include "Cooperation.h"
#include "utils/net.h"
#include "utils/message_helper.h"

#include "protocol/pair.pb.h"
#include "protocol/cooperation.pb.h"

Machine::Machine(Cooperation &cooperation,
                 Glib::RefPtr<DBus::Service> service,
                 uint32_t id,
                 const Glib::ustring &ip,
                 uint16_t port,
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
    , m_methodRequestCooperate(
          new DBus::Method("RequestCooperate",
                           DBus::Method::warp(this, &Machine::requestCooperate),
                           {}))
    , m_ip(ip)
    , m_propertyIP(new DBus::Property("IP", "s", DBus::Property::warp(this, &Machine::getIP)))
    , m_port(port)
    , m_propertyPort(new DBus::Property("Port", "q", DBus::Property::warp(this, &Machine::getPort)))
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
    m_interface->exportMethod(m_methodRequestCooperate);
    m_interface->exportProperty(m_propertyIP);
    m_interface->exportProperty(m_propertyPort);
    m_interface->exportProperty(m_propertyUUID);
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

void Machine::pair([[maybe_unused]] const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    m_sock = Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                 Gio::SocketType::SOCKET_TYPE_STREAM,
                                 Gio::SocketProtocol::SOCKET_PROTOCOL_TCP);
    m_sock->connect(Net::makeSocketAddress(m_ip, m_port));
    Gio::signal_socket().connect(sigc::mem_fun(this, &Machine::dispatcher), m_sock, Glib::IO_IN);

    Message msg;
    PairRequest *request = msg.mutable_pairrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_cooperation.uuid());
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    request->mutable_deviceinfo()->set_compositor(Compositor::NONE);
    MessageHelper::send_message(m_sock, msg);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::onPair(Glib::RefPtr<Gio::Socket> conn) {
    m_sock = conn;
    Gio::signal_socket().connect(sigc::mem_fun(this, &Machine::dispatcher), m_sock, Glib::IO_IN);

    m_paired = true;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    Message msg;
    PairResponse *response = msg.mutable_pairresponse();
    response->set_key(SCAN_KEY);
    response->mutable_deviceinfo()->set_uuid(m_cooperation.uuid());
    response->mutable_deviceinfo()->set_name(Net::getHostname());
    response->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    response->mutable_deviceinfo()->set_compositor(Compositor::NONE);
    response->set_agree(true); // TODO: 询问用户是否同意
    MessageHelper::send_message(m_sock, msg);
}

void Machine::sendFile(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> filepath;
    args.get_child(filepath, 0);

    // TODO: impl

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::requestCooperate(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_sock->is_connected()) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "connect first"});
        return;
    }

    Message msg;
    msg.mutable_cooperaterequest();

    MessageHelper::send_message(m_sock, msg);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::getIP(Glib::VariantBase &property,
                    [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<Glib::ustring>::create(m_ip);
}

void Machine::getPort(Glib::VariantBase &property,
                      [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint16_t>::create(m_port);
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

bool Machine::dispatcher([[maybe_unused]] Glib::IOCondition cond) noexcept {
    auto base = MessageHelper::recv_message(m_sock);
    switch (base.payload_case()) {
    case Message::PayloadCase::kPairResponse: {
        handlePairResponse(base.pairresponse());
        break;
    }

    case Message::PayloadCase::kCooperateRequest: {
        handleCooperateRequest();
        break;
    }

    case Message::PayloadCase::kCooperateResponse: {
        break;
    }

    case Message::PayloadCase::kInputEventRequest: {
        const InputEventRequest &event = base.inputeventrequest();
        m_cooperation.handleReceivedInputEventRequest(event);

        Message msg;
        InputEventResponse *response = msg.mutable_inputeventresponse();
        response->set_serial(event.serial());
        response->set_success(true);
        MessageHelper::send_message(m_sock, msg);
        break;
    }

    case Message::PayloadCase::kInputEventResponse: {
        break;
    }

    case Message::PayloadCase::PAYLOAD_NOT_SET: {
        m_sock->close();
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
        m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));
        return;
    }

    // TODO: handle not agree
}

void Machine::handleCooperateRequest() {
    bool accept = m_cooperation.handleReceivedCooperateRequest(shared_from_this());

    Message msg;
    CooperateResponse *resp = msg.mutable_cooperateresponse();
    resp->set_accept(accept);
    MessageHelper::send_message(m_sock, msg);
}

void Machine::handleInputEvent(const InputEventRequest &event) {
    Message msg;
    msg.set_allocated_inputeventrequest(event.New());
    MessageHelper::send_message(m_sock, msg);
}
