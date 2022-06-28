#include "Machine.h"

#include <condition_variable>

#include "Cooperation.h"
#include "Request.h"
#include "utils/net.h"
#include "utils/message_helper.h"

#include "protocol/pair.pb.h"
#include "protocol/cooperation.pb.h"
#include "protocol/fs.pb.h"

#include "uvxx/TCP.h"
#include "uvxx/Loop.h"
#include "uvxx/Addr.h"
#include "uvxx/Async.h"
#include "uvxx/Signal.h"

Machine::Machine(Cooperation *cooperation,
                 const std::shared_ptr<uvxx::Loop> &loop,
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
    , m_methodDisconnect(
          new DBus::Method("Disconnect", DBus::Method::warp(this, &Machine::disconnect)))
    , m_methodSendFile(new DBus::Method("SendFile",
                                        DBus::Method::warp(this, &Machine::sendFile),
                                        {{"filepath", "s"}}))
    , m_methodRequestCooperate(
          new DBus::Method("RequestCooperate",
                           DBus::Method::warp(this, &Machine::requestCooperate)))
    , m_methodStopCooperation(
          new DBus::Method("StopCooperation", DBus::Method::warp(this, &Machine::stopCooperation)))
    , m_methodFlowTo(new DBus::Method("FlowTo",
                                      DBus::Method::warp(this, &Machine::flowTo),
                                      {{"direction", "q"}, {"x", "q"}, {"y", "q"}}))
    , m_methodMountFs(
          new DBus::Method("MountFs", DBus::Method::warp(this, &Machine::mountFs), {{"path", "s"}}))
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
    , m_propertyCompositor(new DBus::Property("Compositor",
                                              "u",
                                              DBus::Property::warp(this, &Machine::getCompositor)))
    , m_cooperating(false)
    , m_propertyCooperating(
          new DBus::Property("Cooperating",
                             "b",
                             DBus::Property::warp(this, &Machine::getCooperating)))
    , m_direction(FlowDirection::Right)
    , m_propertyDirection(
          new DBus::Property("Direction", "q", DBus::Property::warp(this, &Machine::getDirection)))
    , m_uvLoop(loop)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop)) {

    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodDisconnect);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportMethod(m_methodRequestCooperate);
    m_interface->exportMethod(m_methodStopCooperation);
    m_interface->exportMethod(m_methodFlowTo);
    m_interface->exportMethod(m_methodMountFs);
    m_interface->exportProperty(m_propertyIP);
    m_interface->exportProperty(m_propertyPort);
    m_interface->exportProperty(m_propertyUUID);
    m_interface->exportProperty(m_propertyName);
    m_interface->exportProperty(m_propertyPaired);
    m_interface->exportProperty(m_propertyOS);
    m_interface->exportProperty(m_propertyCompositor);
    m_interface->exportProperty(m_propertyCooperating);
    m_interface->exportProperty(m_propertyDirection);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);
}

Machine::~Machine() {
    m_service->unexportObject(m_object->path());
}

void Machine::pair([[maybe_unused]] const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    m_async->wake([this, invocation]() {
        m_conn = std::make_shared<uvxx::TCP>(m_uvLoop);

        m_conn->onConnected([this, invocation]() {
            spdlog::info("connected");

            m_conn->onClosed(uvxx::memFunc(this, &Machine::handleDisconnected));
            m_conn->onReceived(uvxx::memFunc(this, &Machine::dispatcher));
            m_conn->startRead();

            Message msg;
            auto *request = msg.mutable_pairrequest();
            request->set_key(SCAN_KEY);
            request->mutable_deviceinfo()->set_uuid(m_cooperation->uuid());
            request->mutable_deviceinfo()->set_name(Net::getHostname());
            request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
            request->mutable_deviceinfo()->set_compositor(Compositor::NONE);

            m_conn->write(MessageHelper::genMessage(msg));

            invocation->return_value(Glib::VariantContainerBase{});
        });
        m_conn->onConnectFailed(
            [invocation]([[maybe_unused]] const std::string &title, const std::string &msg) {
                spdlog::info("connect failed: {}", msg);
                invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::FAILED, msg});
            });
        m_conn->connect(uvxx::IPv4Addr::create(m_ip, m_port));
    });
}

void Machine::onPair(const std::shared_ptr<uvxx::TCP> &sock) {
    spdlog::info("onPair");
    m_conn = sock;
    m_conn->onClosed(uvxx::memFunc(this, &Machine::handleDisconnected));
    m_conn->onReceived(uvxx::memFunc(this, &Machine::dispatcher));

    m_paired = true;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    Message msg;
    auto *response = msg.mutable_pairresponse();
    response->set_key(SCAN_KEY);
    response->mutable_deviceinfo()->set_uuid(m_cooperation->uuid());
    response->mutable_deviceinfo()->set_name(Net::getHostname());
    response->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    response->mutable_deviceinfo()->set_compositor(Compositor::NONE);
    response->set_agree(true); // TODO: 询问用户是否同意

    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::disconnect([[maybe_unused]] const Glib::VariantContainerBase &args,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "not connected"});
        return;
    }

    m_conn->close();
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
    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "connect first"});
        return;
    }

    m_async->wake([this]() {
        Message msg;
        msg.mutable_cooperaterequest();
        m_conn->write(MessageHelper::genMessage(msg));
    });

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::stopCooperation(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    invocation->return_value(Glib::VariantContainerBase{});

    if (!m_cooperating) {
        return;
    }

    m_async->wake([this]() {
        Message msg;
        msg.mutable_cooperatestoprequest();
        m_conn->write(MessageHelper::genMessage(msg));
    });

    stopCooperationAux();
}

void Machine::flowTo([[maybe_unused]] const Glib::VariantContainerBase &args,
                     const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<uint16_t> directionV;
    Glib::Variant<uint16_t> xV;
    Glib::Variant<uint16_t> yV;
    args.get_child(directionV, 0);
    args.get_child(xV, 1);
    args.get_child(yV, 2);

    FlowDirection direction = static_cast<FlowDirection>(directionV.get());
    uint16_t x = xV.get();
    uint16_t y = yV.get();

    m_cooperation->handleFlowOut(weak_from_this());

    m_async->wake([this, direction, x, y]() {
        Message msg;
        FlowRequest *flow = msg.mutable_flowrequest();
        flow->set_direction(FlowDirection(direction));
        flow->set_x(x);
        flow->set_y(y);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::mountFs(const Glib::VariantContainerBase &args,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "connect first"});
        return;
    }

    Glib::Variant<Glib::ustring> path;
    args.get_child(path, 0);

    m_async->wake([this, path = path.get()]() {
        Message msg;
        auto *request = msg.mutable_fsrequest();
        request->set_path(path);
        m_conn->write(MessageHelper::genMessage(msg));
    });

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

void Machine::getCooperating(Glib::VariantBase &property,
                             [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<bool>::create(m_cooperating);
}

void Machine::getDirection(Glib::VariantBase &property,
                           [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint16_t>::create(m_direction);
}

void Machine::handleDisconnected() {
    m_cooperating = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));
    m_paired = false;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    m_conn.reset();
}

void Machine::dispatcher(std::shared_ptr<char[]> buffer, ssize_t size) noexcept {
    spdlog::info("received packet from name: {}, UUID: {}, size: {}",
                 std::string(m_name),
                 std::string(m_uuid),
                 size);
    char *buff = buffer.get();

    while (size >= header_size) {
        const auto header = MessageHelper::parseMessageHeader(buff);
        buff += header_size;
        size -= header_size;

        if (size < 0 || static_cast<size_t>(size) < header.size) {
            spdlog::error("wrong size: {}", size);
            break;
        }

        auto msg = MessageHelper::parseMessageBody(buff, header.size);
        spdlog::info("received packet, type: {}", msg.payload_case());

        buff += header.size;
        size -= header.size;

        switch (msg.payload_case()) {
        case Message::PayloadCase::kPairResponse: {
            handlePairResponse(msg.pairresponse());
            break;
        }

        case Message::PayloadCase::kCooperateRequest: {
            handleCooperateRequest();
            break;
        }

        case Message::PayloadCase::kCooperateResponse: {
            const auto &resp = msg.cooperateresponse();
            if (!resp.accept()) {
                // TODO: handle not accepted
                break;
            }

            handleCooperateRequestAccepted();
            break;
        }

        case Message::PayloadCase::kCooperateStopRequest: {
            stopCooperationAux();
            break;
        }

        case Message::PayloadCase::kCooperateStopResponse: {
            break;
        }

        case Message::PayloadCase::kInputEventRequest: {
            const auto &event = msg.inputeventrequest();

            m_cooperation->handleReceivedInputEventRequest(event);

            Message resp;
            InputEventResponse *response = resp.mutable_inputeventresponse();
            response->set_serial(event.serial());
            response->set_success(true);

            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }

        case Message::PayloadCase::kInputEventResponse: {
            break;
        }

        case Message::PayloadCase::kFlowRequest: {
            const auto &req = msg.flowrequest();

            m_cooperation->handleFlowBack(req.direction(), req.x(), req.y());
            break;
        }

        case Message::PayloadCase::kFlowResponse: {
            break;
        }

        case Message::PayloadCase::kFsRequest: {
            const auto &req = msg.fsrequest();

            m_cooperation->handleReceivedFsRequest(this, req);
            break;
        }

        case Message::PayloadCase::kFsResponse: {
            const auto &resp = msg.fsresponse();

            m_cooperation->handleReceivedFsResponse(this, resp);
            break;
        }

        default: {
            spdlog::warn("invalid message type: {}", msg.payload_case());
            m_conn->close();
            return;
            break;
        }
        }
    }
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
    m_cooperation->handleReceivedCooperateRequest(this);
}

void Machine::handleCooperateRequestAccepted() {
    m_cooperating = true;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));

    m_direction = FlowDirection::Right;
    m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));

    m_cooperation->handleRemoteAcceptedCooperation();
}

void Machine::handleStopCooperation() {
    m_cooperation->handleStopCooperation();
}

void Machine::handleInputEvent(const InputEventRequest &event) {
    Message msg;
    msg.mutable_inputeventrequest()->CopyFrom(event);

    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::setCooperationRequest(const std::shared_ptr<Request> &req) {
    m_cooperationRequest = req;

    m_cooperationRequest->onAccept().connect(
        sigc::mem_fun(this, &Machine::handleAcceptCooperation));
}

void Machine::setFilesystemRequest(const std::shared_ptr<Request> &req) {
    m_filesystemRequest = req;

    m_filesystemRequest->onAccept().connect(sigc::mem_fun(this, &Machine::handleAcceptFilesystem));
};

void Machine::handleAcceptCooperation(
    bool accepted,
    [[maybe_unused]] const std::map<Glib::ustring, Glib::VariantBase> &hint,
    [[maybe_unused]] uint32_t serial) {
    m_async->wake([this, accepted]() {
        Message msg;
        CooperateResponse *resp = msg.mutable_cooperateresponse();
        resp->set_accept(accepted);
        m_conn->write(MessageHelper::genMessage(msg));

        m_cooperating = true;
        m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));

        m_direction = FlowDirection::Left;
        m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));

        if (accepted) {
            auto wptr = weak_from_this();
            m_cooperation->handleStartCooperation(wptr);
        }
    });
}

void Machine::handleAcceptFilesystem(bool accepted,
                                     const std::map<Glib::ustring, Glib::VariantBase> &hint,
                                     uint32_t serial) {
    m_async->wake([this, accepted, hint, serial]() {
        auto port = *static_cast<const uint16_t *>(hint.at("port").get_data());

        Message msg;
        FsResponse *resp = msg.mutable_fsresponse();
        resp->set_accepted(accepted);
        resp->set_port(port);
        resp->set_serial(serial);

        m_conn->write(MessageHelper::genMessage(msg));
    });
}

void Machine::stopCooperationAux() {
    m_cooperation->handleStopCooperation();

    m_cooperating = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));
}
