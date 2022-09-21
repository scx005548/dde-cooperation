#include "Machine.h"

#include <condition_variable>

#include "Manager.h"
#include "ClipboardBase.h"
#include "Request.h"
#include "InputEmittorWrapper.h"
#include "FuseServer.h"
#include "FuseClient.h"
#include "utils/net.h"
#include "utils/message_helper.h"

#include "protocol/message.pb.h"

#include "uvxx/TCP.h"
#include "uvxx/Loop.h"
#include "uvxx/Addr.h"
#include "uvxx/Async.h"
#include "uvxx/Signal.h"

namespace fs = std::filesystem;

static const std::string fileSchema{"file://"};

Machine::Machine(Manager *manager,
                 ClipboardBase *clipboard,
                 const std::shared_ptr<uvxx::Loop> &uvLoop,
                 Glib::RefPtr<DBus::Service> service,
                 uint32_t id,
                 const fs::path &dataDir,
                 const Glib::ustring &ip,
                 uint16_t port,
                 const DeviceInfo &sp)
    : m_manager(manager)
    , m_clipboard(clipboard)
    , m_dataDir(dataDir)
    , m_mountpoint(m_dataDir / "mp")
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
    , m_uvLoop(uvLoop)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_mounted(false) {

    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodDisconnect);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportMethod(m_methodRequestCooperate);
    m_interface->exportMethod(m_methodStopCooperation);
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

    m_inputEmittors.emplace(
        std::make_pair(InputDeviceType::KEYBOARD,
                       std::make_unique<InputEmittorWrapper>(weak_from_this(),
                                                             m_uvLoop,
                                                             InputDeviceType::KEYBOARD)));
    m_inputEmittors.emplace(std::make_pair(
        InputDeviceType::MOUSE,
        std::make_unique<InputEmittorWrapper>(weak_from_this(), m_uvLoop, InputDeviceType::MOUSE)));
    m_inputEmittors.emplace(
        std::make_pair(InputDeviceType::TOUCHPAD,
                       std::make_unique<InputEmittorWrapper>(weak_from_this(),
                                                             m_uvLoop,
                                                             InputDeviceType::TOUCHPAD)));
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

            initConnection();
            m_conn->startRead();

            Message msg;
            auto *request = msg.mutable_pairrequest();
            request->set_key(SCAN_KEY);
            request->mutable_deviceinfo()->set_uuid(m_manager->uuid());
            request->mutable_deviceinfo()->set_name(Net::getHostname());
            request->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
            request->mutable_deviceinfo()->set_compositor(Compositor::CPST_NONE);

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
    initConnection();

    m_paired = true;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    Message msg;
    auto *response = msg.mutable_pairresponse();
    response->set_key(SCAN_KEY);
    response->mutable_deviceinfo()->set_uuid(m_manager->uuid());
    response->mutable_deviceinfo()->set_name(Net::getHostname());
    response->mutable_deviceinfo()->set_os(DeviceOS::LINUX);
    response->mutable_deviceinfo()->set_compositor(Compositor::CPST_NONE);
    response->set_agree(true); // TODO: 询问用户是否同意

    m_conn->write(MessageHelper::genMessage(msg));

    mountFs("/");
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

    m_async->wake([this, path = std::string(filepath.get())]() {
        Message msg;
        FsSendFileRequest *send = msg.mutable_fssendfilerequest();
        send->set_path(path);
        m_conn->write(MessageHelper::genMessage(msg));
    });

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

void Machine::initConnection() {
    m_conn->onClosed(uvxx::memFunc(this, &Machine::handleDisconnected));
    m_conn->onReceived(uvxx::memFunc(this, &Machine::dispatcher));
    m_conn->tcpNoDelay();
    m_conn->keepalive(true, 20);
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

    m_async->wake([this, path = path.get()]() { mountFs(path); });

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

void Machine::mountFs(const std::string &path) {
    Message msg;
    auto *request = msg.mutable_fsrequest();
    request->set_path(path);
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::handleDisconnected() {
    m_cooperating = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));
    m_paired = false;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    m_conn.reset();
}

void Machine::dispatcher(uvxx::Buffer &buff) noexcept {
    spdlog::info("received packet from name: {}, UUID: {}, size: {}",
                 std::string(m_name),
                 std::string(m_uuid),
                 buff.size());

    while (buff.size() >= header_size) {
        auto res = MessageHelper::parseMessage<Message>(buff);
        if (!res.has_value()) {
            return;
        }

        Message &msg = res.value();

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
            handleCooperateResponse(msg.cooperateresponse());
            break;
        }

        case Message::PayloadCase::kCooperateStopRequest: {
            handleStopCooperationRequest();
            break;
        }

        case Message::PayloadCase::kCooperateStopResponse: {
            break;
        }

        case Message::PayloadCase::kInputEventRequest: {
            handleInputEventRequest(msg.inputeventrequest());
            break;
        }

        case Message::PayloadCase::kInputEventResponse: {
            break;
        }

        case Message::PayloadCase::kFlowRequest: {
            handleFlowRequest(msg.flowrequest());
            break;
        }

        case Message::PayloadCase::kFlowResponse: {
            break;
        }

        case Message::PayloadCase::kFsSendFileRequest: {
            // const auto &req = msg.fssendfilerequest();

            // m_manager->handleReceivedSendFileRequest(this, req);
            break;
        }

        case Message::PayloadCase::kFsSendFileResponse: {
            break;
        }

        case Message::PayloadCase::kFsRequest: {
            handleFsRequest(msg.fsrequest());

            // TODO:
            // m_manager->handleReceivedFsRequest(this, req);
            break;
        }

        case Message::PayloadCase::kFsResponse: {
            handleFsResponse(msg.fsresponse());
            break;
        }

        case Message::PayloadCase::kClipboardNotify: {
            handleClipboardNotify(msg.clipboardnotify());
            break;
        }

        case Message::PayloadCase::kClipboardGetContent: {
            handleClipboardGetContent(msg.clipboardgetcontent());
            break;
        }

        case Message::PayloadCase::kClipboardGetContentReply: {
            handleClipboardGetContentReply(msg.clipboardgetcontentreply());
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
    if (!agree) {
        // TODO: handle not agree
        return;
    }

    m_paired = true;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    mountFs("/");
}

void Machine::handleCooperateRequest() {
    bool accepted = true;
    m_async->wake([this, accepted]() {
        Message msg;
        CooperateResponse *resp = msg.mutable_cooperateresponse();
        resp->set_accept(accepted);
        m_conn->write(MessageHelper::genMessage(msg));

        if (accepted) {
            auto wptr = weak_from_this();
            m_manager->onStartCooperation(wptr, false);

            m_cooperating = true;
            m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));

            m_direction = FlowDirection::Left;
            m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));
        }
    });
}

void Machine::handleCooperateResponse(const CooperateResponse &resp) {
    if (!resp.accept()) {
        return;
    }

    m_cooperating = true;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));

    m_direction = FlowDirection::Right;
    m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));

    m_manager->onStartCooperation(weak_from_this(), true);
}

void Machine::handleStopCooperationRequest() {
    stopCooperationAux();
}

void Machine::handleInputEventRequest(const InputEventRequest &req) {
    spdlog::debug("received input event");

    bool success = true;

    auto deviceType = static_cast<InputDeviceType>(req.devicetype());
    auto it = m_inputEmittors.find(deviceType);
    if (it == m_inputEmittors.end()) {
        success = false;
        spdlog::error("no deviceType {} found", static_cast<uint8_t>(deviceType));
    } else {
        auto &inputEmittor = it->second;
        success = inputEmittor->emitEvent(req.type(), req.code(), req.value());
    }

    Message resp;
    InputEventResponse *response = resp.mutable_inputeventresponse();
    response->set_serial(req.serial());
    response->set_success(success);

    m_conn->write(MessageHelper::genMessage(resp));
}

void Machine::handleFlowRequest(const FlowRequest &req) {
    m_manager->onFlowBack(req.direction(), req.x(), req.y());
}

void Machine::handleFsRequest([[maybe_unused]] const FsRequest &req) {
    m_fuseServer = std::make_unique<FuseServer>(weak_from_this(), m_uvLoop);
    uint16_t port = m_fuseServer->port();

    // TODO: request accept
    Message msg;
    auto *fsresponse = msg.mutable_fsresponse();
    fsresponse->set_accepted(true);
    fsresponse->set_port(port);
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::handleFsResponse(const FsResponse &resp) {
    if (!resp.accepted()) {
        return;
    }

    m_fuseClient = std::make_unique<FuseClient>(m_uvLoop, m_ip, resp.port(), m_mountpoint);
}

void Machine::handleClipboardNotify(const ClipboardNotify &notify) {
    auto &targetsp = notify.targets();
    std::vector<std::string> targets{targetsp.cbegin(), targetsp.cend()};
    m_manager->onMachineOwnClipboard(weak_from_this(), targets);
}

void Machine::handleClipboardGetContent(const ClipboardGetContent &req) {
    auto target = req.target();
    auto cb = [this, target](const std::vector<char> &content) {
        Message msg;
        auto *reply = msg.mutable_clipboardgetcontentreply();
        reply->set_target(target);
        reply->set_content(std::string(content.begin(), content.end()));
        m_conn->write(MessageHelper::genMessage(msg));
    };
    m_clipboard->readTargetContent(target, cb);
}

void Machine::handleClipboardGetContentReply(const ClipboardGetContentReply &resp) {
    auto target = resp.target();
    auto content = resp.content();
    if (target == "x-special/gnome-copied-files") {
        spdlog::warn("ori x-special/gnome-copied-files: {}", content);
    }
    if (m_clipboard->isFiles()) {
        std::stringstream ss(content);
        std::string out;
        std::string line;
        while (!ss.eof()) {
            std::getline(ss, line, '\n');
            if (line[0] == '/') { // starts with '/'
                out.reserve(out.length() + m_mountpoint.string().length() + line.length() + 1);
                out.append(m_mountpoint.string());
                out.append(line);
            } else if (line.rfind(fileSchema, 0) == 0) { // starts with 'file://'
                out.reserve(out.length() + m_mountpoint.string().length() + line.length() + 1);
                out.append(fileSchema);
                out.append(m_mountpoint.string());
                out.append(line.begin() + fileSchema.length(), line.end());
            } else {
                out.reserve(out.length() + line.length() + 1);
                out.append(line);
            }
            out.push_back('\n');
        }
        out.resize(out.length() - 1);
        content.swap(out);
        // spdlog::info("content[{}]: {}", content.length(), content);
    }
    if (target == "x-special/gnome-copied-files") {
        spdlog::warn("x-special/gnome-copied-files: {}", content);
    }
    m_clipboard->updateTargetContent(target, std::vector<char>(content.begin(), content.end()));
}

void Machine::onInputGrabberEvent(uint8_t deviceType,
                                  unsigned int type,
                                  unsigned int code,
                                  int value) {
    Message msg;
    auto *inputEvent = msg.mutable_inputeventrequest();
    inputEvent->set_devicetype(static_cast<DeviceType>(deviceType));
    inputEvent->set_type(type);
    inputEvent->set_code(code);
    inputEvent->set_value(value);
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::onClipboardTargetsChanged(const std::vector<std::string> &targets) {
    Message msg;
    auto *clipboardNotify = msg.mutable_clipboardnotify();
    *(clipboardNotify->mutable_targets()) = {targets.cbegin(), targets.cend()};
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept {
    Message msg;
    FlowRequest *flow = msg.mutable_flowrequest();
    flow->set_direction(FlowDirection(direction));
    flow->set_x(x);
    flow->set_y(y);
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::readTarget(const std::string &target) {
    Message msg;
    auto *clipboardGetContent = msg.mutable_clipboardgetcontent();
    clipboardGetContent->set_target(target);
    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::handleAcceptSendFile(
    bool accepted,
    [[maybe_unused]] const std::map<Glib::ustring, Glib::VariantBase> &hint,
    [[maybe_unused]] uint32_t serial) {

    m_async->wake([this, accepted, hint, serial]() {
        Message msg;

        FsSendFileResponse *resp = msg.mutable_fssendfileresponse();
        resp->set_accepted(accepted);
        resp->set_serial(serial);
        m_conn->write(MessageHelper::genMessage(msg));

        if (!m_mounted) {
            FsRequest *req = msg.mutable_fsrequest();
            req->set_path("/");
            m_conn->write(MessageHelper::genMessage(msg));
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
    m_manager->onStopCooperation();

    m_cooperating = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_cooperating));
}
