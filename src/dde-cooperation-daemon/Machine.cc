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
#include "ConfirmDialogWrapper.h"

#include "protocol/message.pb.h"

#include "uvxx/TCP.h"
#include "uvxx/Loop.h"
#include "uvxx/Timer.h"
#include "uvxx/Addr.h"
#include "uvxx/Async.h"
#include "uvxx/Process.h"
#include "uvxx/Signal.h"

namespace fs = std::filesystem;

static const std::string fileSchema{"file://"};

static const uint64_t U10s = 10 * 1000;
static const uint64_t U25s = 25 * 1000;

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
    , m_deviceSharing(false)
    , m_propertyCooperating(
          new DBus::Property("Cooperating",
                             "b",
                             DBus::Property::warp(this, &Machine::getCooperating)))
    , m_direction(FLOW_DIRECTION_RIGHT)
    , m_propertyDirection(
          new DBus::Property("Direction", "q", DBus::Property::warp(this, &Machine::getDirection)))
    , m_uvLoop(uvLoop)
    , m_pingTimer(std::make_shared<uvxx::Timer>(m_uvLoop, uvxx::memFunc(this, &Machine::ping)))
    , m_offlineTimer(
          std::make_shared<uvxx::Timer>(m_uvLoop, uvxx::memFunc(this, &Machine::onOffline)))
    , m_mounted(false)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_object(new DBus::Object(m_path)) {

    m_interface->exportMethod(m_methodPair);
    m_interface->exportMethod(m_methodDisconnect);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportMethod(m_methodRequestCooperate);
    m_interface->exportMethod(m_methodStopCooperation);
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

    m_pingTimer->start(U10s);
    m_offlineTimer->oneshot(U25s);
}

Machine::~Machine() {
    m_pingTimer->close();
    m_offlineTimer->close();
    m_async->close();
    if (m_conn) {
        m_conn->close();
    }

    m_service->unexportObject(m_object->path());
}

void Machine::init() {
    m_service->exportObject(m_object);
}

bool Machine::isPcMachine() const {
    return m_os == DEVICE_OS_UOS || m_os == DEVICE_OS_LINUX
           || m_os == DEVICE_OS_WINDOWS || m_os == DEVICE_OS_MACOS;
}

bool Machine::isAndroid() const {
    return m_os == DEVICE_OS_ANDROID;
}

void Machine::pair([[maybe_unused]] const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (m_paired) {
        invocation->return_value(Glib::VariantContainerBase{});
        return;
    }

    m_async->wake([this, invocation]() {
        m_conn = std::make_shared<uvxx::TCP>(m_uvLoop);

        m_conn->onConnected([this, invocation]() {
            spdlog::info("connected");

            initConnection();
            m_conn->startRead();

            m_pingTimer->stop();
            m_offlineTimer->stop();

            Message msg;
            auto *request = msg.mutable_pairrequest();
            request->set_key(SCAN_KEY);
            request->mutable_deviceinfo()->set_uuid(m_manager->uuid());
            request->mutable_deviceinfo()->set_name(Net::getHostname());
            request->mutable_deviceinfo()->set_os(DEVICE_OS_LINUX);
            request->mutable_deviceinfo()->set_compositor(COMPOSITOR_X11);

            sendMessage(msg);

            invocation->return_value(Glib::VariantContainerBase{});
        });
        m_conn->onConnectFailed(
            [this, invocation]([[maybe_unused]] const std::string &title, const std::string &msg) {
                spdlog::info("connect failed: {}", msg);
                invocation->return_error(Gio::DBus::Error{Gio::DBus::Error::FAILED, msg});

                // TODO tips and send scan
                m_manager->ping(m_ip);
            });
        m_conn->connect(uvxx::IPv4Addr::create(m_ip, m_port));
    });
}

void Machine::updateMachineInfo(const Glib::ustring &ip, uint16_t port, const DeviceInfo &devInfo) {
    m_ip = ip;
    m_port = port;
    m_name = devInfo.name();
    m_compositor = devInfo.compositor();
}

void Machine::receivedPing() {
    m_offlineTimer->reset();
    m_pingTimer->reset();
}

void Machine::onPair(const std::shared_ptr<uvxx::TCP> &sock) {
    spdlog::info("request onPair");
    m_conn = sock;
    initConnection();

    m_pingTimer->stop();
    m_offlineTimer->stop();

    m_confirmDialog = std::make_unique<ConfirmDialogWrapper>(
        m_ip,
        m_name,
        m_uvLoop,
        uvxx::memFunc(this, &Machine::receivedUserConfirm));
}

void Machine::disconnect([[maybe_unused]] const Glib::VariantContainerBase &args,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "not connected"});
        return;
    }

    m_async->wake([this, &invocation]() {
        m_conn->close();

        invocation->return_value(Glib::VariantContainerBase{});
    });
}

void Machine::sendFile(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> filepath;
    args.get_child(filepath, 0);

    std::vector<Glib::ustring> files;
    files.push_back(filepath.get());
    sendFiles(files);

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::requestCooperate(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (m_deviceSharing) {
        invocation->return_value(Glib::VariantContainerBase{});
        return;
    }

    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "connect first"});
        return;
    }

    m_async->wake([this]() {
        Message msg;
        msg.mutable_devicesharingstartrequest();
        sendMessage(msg);
    });

    invocation->return_value(Glib::VariantContainerBase{});
}

void Machine::stopCooperation(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    invocation->return_value(Glib::VariantContainerBase{});

    if (!m_deviceSharing) {
        return;
    }

    m_async->wake([this]() {
        Message msg;
        msg.mutable_devicesharingstoprequest();
        sendMessage(msg);
    });

    stopDeviceSharingAux();
}

void Machine::ping() {
    m_manager->ping(m_ip);
}

void Machine::onOffline() {
    m_manager->onMachineOffline(m_uuid);
}

void Machine::initConnection() {
    m_conn->onClosed(uvxx::memFunc(this, &Machine::handleDisconnected));
    m_conn->onReceived(uvxx::memFunc(this, &Machine::dispatcher));
    m_conn->tcpNoDelay();
    m_conn->keepalive(true, 20);
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
    property = Glib::Variant<bool>::create(m_deviceSharing);
}

void Machine::getDirection(Glib::VariantBase &property,
                           [[maybe_unused]] const Glib::ustring &propertyName) const {
    property = Glib::Variant<uint16_t>::create(m_direction);
}

void Machine::handleDisconnected() {
    spdlog::info("disconnected");

    m_manager->onStopDeviceSharing();

    m_deviceSharing = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_deviceSharing));
    m_paired = false;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    if (m_fuseClient) {
        m_fuseClient->exit();
        m_fuseClient.reset();
    }

    if (m_fuseServer) {
        m_fuseServer.reset();
    }

    m_conn.reset();

    m_pingTimer->reset();
}

void Machine::dispatcher(uvxx::Buffer &buff) noexcept {
    spdlog::info("received packet from name: {}, UUID: {}, size: {}",
                 std::string(m_name),
                 std::string(m_uuid),
                 buff.size());

    while (buff.size() >= header_size) {
        auto res = MessageHelper::parseMessage<Message>(buff);
        if (!res.has_value()) {
            if (res.error() == MessageHelper::PARSE_ERROR::ILLEGAL_MESSAGE) {
                spdlog::error("illegal message from {}, close the connection", std::string(m_uuid));
                m_conn->close();
            }
            return;
        }

        Message &msg = res.value();
        spdlog::debug("message type: {}", msg.payload_case());

        switch (msg.payload_case()) {
        case Message::PayloadCase::kPairResponse: {
            handlePairResponseAux(msg.pairresponse());
            break;
        }

        case Message::PayloadCase::kDeviceSharingStartRequest: {
            handleDeviceSharingStartRequest();
            break;
        }

        case Message::PayloadCase::kDeviceSharingStartResponse: {
            handleDeviceSharingStartResponse(msg.devicesharingstartresponse());
            break;
        }

        case Message::PayloadCase::kDeviceSharingStopRequest: {
            handleDeviceSharingStopRequest();
            break;
        }

        case Message::PayloadCase::kDeviceSharingStopResponse: {
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

        case Message::PayloadCase::kFsRequest: {
            handleFsRequest(msg.fsrequest());
            break;
        }

        case Message::PayloadCase::kFsResponse: {
            handleFsResponse(msg.fsresponse());
            break;
        }

        case Message::PayloadCase::kFsSendFileRequest: {
            handleFsSendFileRequest(msg.fssendfilerequest());
            break;
        }

        case Message::PayloadCase::kFsSendFileResponse: {
            break;
        }

        case Message::PayloadCase::kFsSendFileResult: {
            break;
        }

        case Message::PayloadCase::kClipboardNotify: {
            handleClipboardNotify(msg.clipboardnotify());
            break;
        }

        case Message::PayloadCase::kClipboardGetContentRequest: {
            handleClipboardGetContentRequest(msg.clipboardgetcontentrequest());
            break;
        }

        case Message::PayloadCase::kClipboardGetContentResponse: {
            handleClipboardGetContentResponse(msg.clipboardgetcontentresponse());
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

void Machine::handlePairResponseAux(const PairResponse &resp) {
    bool agree = resp.agree();
    if (!agree) {
        // handle not agree
        m_conn->close();
        return;
    }

    m_paired = true;
    m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

    handleConnected();
}

void Machine::handleDeviceSharingStartRequest() {
    bool accepted = true;
    m_async->wake([this, accepted]() {
        Message msg;
        DeviceSharingStartResponse *resp = msg.mutable_devicesharingstartresponse();
        resp->set_accept(accepted);
        sendMessage(msg);

        if (accepted) {
            auto wptr = weak_from_this();
            m_manager->onStartDeviceSharing(wptr, false);

            m_deviceSharing = true;
            m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_deviceSharing));

            m_direction = FLOW_DIRECTION_LEFT;
            m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));
        }
    });
}

void Machine::handleDeviceSharingStartResponse(const DeviceSharingStartResponse &resp) {
    if (!resp.accept()) {
        return;
    }

    m_deviceSharing = true;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_deviceSharing));

    m_direction = FLOW_DIRECTION_RIGHT;
    m_propertyDirection->emitChanged(Glib::Variant<uint16_t>::create(m_direction));

    m_manager->onStartDeviceSharing(weak_from_this(), true);
}

void Machine::handleDeviceSharingStopRequest() {
    stopDeviceSharingAux();
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
    if (m_fuseServer) {
        Message msg;
        auto *fsresponse = msg.mutable_fsresponse();
        fsresponse->set_accepted(false);
        fsresponse->set_port(0);
        sendMessage(msg);
        return;
    }

    m_fuseServer = std::make_unique<FuseServer>(weak_from_this(), m_uvLoop);

    // TODO: request accept
    Message msg;
    auto *fsresponse = msg.mutable_fsresponse();
    fsresponse->set_accepted(true);
    fsresponse->set_port(m_fuseServer->port());
    sendMessage(msg);
}

void Machine::handleFsResponse(const FsResponse &resp) {
    if (!resp.accepted()) {
        return;
    }

    m_fuseClient = std::make_unique<FuseClient>(m_uvLoop, m_ip, resp.port(), m_mountpoint);
}

void Machine::handleFsSendFileRequest(const FsSendFileRequest &req) {
    Message msg;
    auto *fssendfileresponse = msg.mutable_fssendfileresponse();
    fssendfileresponse->set_serial(req.serial());

    if (!m_fuseClient) {
        fssendfileresponse->set_accepted(false);
        sendMessage(msg);
        return;
    }

    fssendfileresponse->set_accepted(true);
    sendMessage(msg);

    std::string home = getenv("HOME");
    std::string reqPath = req.path();
    if (!reqPath.empty() && reqPath[0] != '/') {
        reqPath = "/" + reqPath;
    }
    std::string filePath = m_mountpoint.string() + reqPath;
    auto process = std::make_shared<uvxx::Process>(m_uvLoop, "/bin/cp");
    process->args = {"-r", filePath, home};
    process->onExit([this, serial = req.serial(), path = req.path(), process](
                        int64_t exit_status,
                        [[maybe_unused]] int term_signal) {
        Message msg;
        auto *fssendfileresult = msg.mutable_fssendfileresult();
        fssendfileresult->set_serial(serial);
        fssendfileresult->set_path(path);

        if (exit_status != 0) {
            spdlog::info("copy files failed");
        } else {
            spdlog::info("copy files success");
        }

        fssendfileresult->set_result(exit_status == 0);
        sendMessage(msg);

        process->onExit(nullptr);
    });
    process->spawn();
}

void Machine::handleClipboardNotify(const ClipboardNotify &notify) {
    auto &targetsp = notify.targets();
    std::vector<std::string> targets{targetsp.cbegin(), targetsp.cend()};
    m_manager->onMachineOwnClipboard(weak_from_this(), targets);
}

void Machine::handleClipboardGetContentRequest(const ClipboardGetContentRequest &req) {
    auto target = req.target();
    auto cb = [this, target](const std::vector<char> &content) {
        Message msg;
        auto *reply = msg.mutable_clipboardgetcontentresponse();
        reply->set_target(target);
        reply->set_content(std::string(content.begin(), content.end()));
        sendMessage(msg);
    };
    m_clipboard->readTargetContent(target, cb);
}

void Machine::handleClipboardGetContentResponse(const ClipboardGetContentResponse &resp) {
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
    sendMessage(msg);
}

void Machine::onClipboardTargetsChanged(const std::vector<std::string> &targets) {
    Message msg;
    auto *clipboardNotify = msg.mutable_clipboardnotify();
    *(clipboardNotify->mutable_targets()) = {targets.cbegin(), targets.cend()};
    sendMessage(msg);
}

void Machine::flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept {
    Message msg;
    FlowRequest *flow = msg.mutable_flowrequest();
    flow->set_direction(FlowDirection(direction));
    flow->set_x(x);
    flow->set_y(y);
    sendMessage(msg);
}

void Machine::readTarget(const std::string &target) {
    Message msg;
    auto *clipboardGetContent = msg.mutable_clipboardgetcontentrequest();
    clipboardGetContent->set_target(target);
    sendMessage(msg);
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
        sendMessage(msg);

        if (!m_mounted) {
            FsRequest *req = msg.mutable_fsrequest();
            req->set_path("/");
            sendMessage(msg);
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

        sendMessage(msg);
    });
}

void Machine::stopDeviceSharingAux() {
    m_manager->onStopDeviceSharing();

    m_deviceSharing = false;
    m_propertyCooperating->emitChanged(Glib::Variant<bool>::create(m_deviceSharing));
}

void Machine::receivedUserConfirm(uvxx::Buffer &buff) {
    m_confirmDialog.reset();

    if (buff.size() != 1) {
        spdlog::warn("user confirm has error!");
        return;
    }

    bool isAccept = (buff.data()[0] == ACCEPT);
    buff.clear();

    Message msg;
    auto *response = msg.mutable_pairresponse();
    response->set_key(SCAN_KEY);
    response->mutable_deviceinfo()->set_uuid(m_manager->uuid());
    response->mutable_deviceinfo()->set_name(Net::getHostname());
    response->mutable_deviceinfo()->set_os(DEVICE_OS_LINUX);
    response->mutable_deviceinfo()->set_compositor(COMPOSITOR_X11);
    response->set_agree(isAccept); // 询问用户是否同意

    sendMessage(msg);

    if (isAccept) {
        m_paired = true;
        m_propertyPaired->emitChanged(Glib::Variant<bool>::create(m_paired));

        handleConnected();
    }
}

void Machine::sendFiles(const std::vector<Glib::ustring> &filePaths) {
    m_async->wake([this, filePaths]() {
        for (const Glib::ustring &filePath : filePaths) {
            Message msg;
            FsSendFileRequest *send = msg.mutable_fssendfilerequest();
            send->set_path(std::string(filePath));
            sendMessage(msg);
        }
    });
}

void Machine::sendMessage(const Message &msg) {
    if (!m_conn) {
        spdlog::warn("connection reset but still want to send msg:{}", msg.GetTypeName());
        return;
    }

    m_conn->write(MessageHelper::genMessage(msg));
}
