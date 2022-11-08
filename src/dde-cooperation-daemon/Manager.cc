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

#include "config.h"
#include "Machine.h"
#include "PCMachine.h"
#include "AndroidMachine.h"
#include "Request.h"
#include "X11/Display.h"
#include "X11/Clipboard.h"
#include "utils/message_helper.h"
#include "protocol/message.pb.h"
#include "protocol/ipc_message.pb.h"

namespace fs = std::filesystem;

const static fs::path inputDevicePath = "/dev/input";

static fs::path getExecutablePath(uint32_t pid) {
    fs::path exeFilePath = fmt::format("/proc/{}/exe", pid);
    if (!fs::exists(exeFilePath) || !fs::is_symlink(exeFilePath)) {
        return "";
    }

    return fs::read_symlink(exeFilePath);
}

static bool isSubDir(fs::path p, fs::path root) {
    for (;;) {
        if (p == root) {
            return true;
        }
        if (!p.has_parent_path()) {
            return false;
        }
        p = p.parent_path();
    }

    return false;
}

Manager::Manager(const std::shared_ptr<uvxx::Loop> &uvLoop, const std::filesystem::path &dataDir)
    : m_dataDir(dataDir)
    , m_mountRoot(m_dataDir / "mr")
    , m_lastMachineIndex(0)
    , m_deviceSharingSwitch(true)
    , m_lastRequestId(0)
    , m_uvLoop(uvLoop)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_socketScan(std::make_shared<uvxx::UDP>(m_uvLoop))
    , m_listenPair(std::make_shared<uvxx::TCP>(m_uvLoop))
    , m_bus(Gio::DBus::Connection::get_sync(Gio::DBus::BusType::BUS_TYPE_SESSION))
    , m_service(new DBus::Service{"com.deepin.Cooperation", Gio::DBus::BusType::BUS_TYPE_SESSION})
    , m_object(new DBus::Object("/com/deepin/Cooperation"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation"))
    , m_methodGetUUID(new DBus::Method("GetUUID",
                                       DBus::Method::warp(this, &Manager::getUUID),
                                       {},
                                       {{"uuid", "s"}}))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Manager::scan)))
    , m_methodKnock(new DBus::Method("Knock",
                                     DBus::Method::warp(this, &Manager::knock),
                                     {{"ip", "s"}, {"port", "i"}}))
    , m_methodSendFile(new DBus::Method("SendFile",
                                        DBus::Method::warp(this, &Manager::sendFile),
                                        {{"filePath", "as"}, {"osType", "i"}}))
    , m_propertyMachines(
          new DBus::Property("Machines", "ao", DBus::Property::warp(this, &Manager::getMachines)))
    , m_propertyDeviceSharingSwitch(
          new DBus::Property("DeviceSharingSwitch",
                             "b",
                             DBus::Property::warp(this, &Manager::getDeviceSharingSwitch),
                             DBus::Property::warp(this, &Manager::setDeviceSharingSwitch)))
    , m_dbusProxy(Gio::DBus::Proxy::Proxy::create_sync(m_bus,
                                                       "org.freedesktop.DBus",
                                                       "/org/freedesktop/DBus",
                                                       "org.freedesktop.DBus"))
    , m_powersaverProxy(Gio::DBus::Proxy::Proxy::create_sync(m_bus,
                                                             "org.freedesktop.ScreenSaver",
                                                             "/org/freedesktop/ScreenSaver",
                                                             "org.freedesktop.ScreenSaver"))
    , m_keypair(m_dataDir, KeyPair::KeyType::ED25519) {
    ensureDataDirExists();
    initUUID();

    m_keypair.load();

    m_service->registerService();
    m_interface->exportMethod(m_methodGetUUID);
    m_interface->exportMethod(m_methodScan);
    m_interface->exportMethod(m_methodKnock);
    m_interface->exportMethod(m_methodSendFile);
    m_interface->exportProperty(m_propertyMachines);
    m_interface->exportProperty(m_propertyDeviceSharingSwitch);
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
    spdlog::debug("TCP listening on port: {}", m_port);

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

    scanAux();
}

Manager::~Manager() {
    m_async->close();
    m_socketScan->close();
    m_listenPair->close();
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

bool Manager::isValidUUID(const std::string &str) const noexcept {
    uuid_t uuid;
    int res = uuid_parse_range(str.data(), str.data() + str.size(), uuid);
    return res == 0;
}

void Manager::getUUID([[maybe_unused]] const Glib::VariantContainerBase &args,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::ustring sender = invocation->get_sender();

    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<Glib::ustring>::create(sender),
    });
    auto res = m_dbusProxy->call_sync("GetConnectionUnixProcessID", params);
    Glib::Variant<uint32_t> vpid;
    res.get_child(vpid);
    uint32_t pid = vpid.get();

    auto callerPath = getExecutablePath(pid);
    if (!isSubDir(callerPath, EXECUTABLE_INSTALL_DIR)) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "Access Denied"});
        return;
    }

    invocation->return_value(Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<Glib::ustring>::create(m_uuid),
    }));
}

void Manager::scan([[maybe_unused]] const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_deviceSharingSwitch) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::FAILED, "DeviceSharing Switch close!"});
        spdlog::debug("DeviceSharing Switch close");
        return;
    }

    m_async->wake([this]() { scanAux(); });

    invocation->return_value(Glib::VariantContainerBase{});
}

void Manager::knock(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<int> port;

    args.get_child(ip, 0);
    args.get_child(port, 1);

    m_async->wake([this, ip = std::string(ip.get()), port = port.get()]() { ping(ip, port); });

    invocation->return_value(Glib::VariantContainerBase{});
}

void Manager::sendFile(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<std::vector<Glib::ustring>> files;
    Glib::Variant<int> osType;

    args.get_child(files, 0);
    args.get_child(osType, 1);

    if (files.get().empty()) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::FAILED, "filepath param has error!"});
        return;
    }

    int dstOs = osType.get();
    bool dstIsPcMachine = dstOs == DEVICE_OS_UOS || dstOs == DEVICE_OS_LINUX
                          || dstOs == DEVICE_OS_WINDOWS || dstOs == DEVICE_OS_MACOS;
    bool dstIsAndroid = dstOs == DEVICE_OS_ANDROID;

    bool hasSend = false;
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;
        if (!machine->m_paired) {
            continue;
        }

        if ((dstIsPcMachine && machine->isPcMachine()) || (dstIsAndroid && machine->isAndroid())) {
            machine->sendFiles(files.get());
            hasSend = true;
            break;
        }
    }

    if (hasSend) {
        invocation->return_value(Glib::VariantContainerBase{});
    } else {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::FAILED, "Target machine not found!"});
    }
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

void Manager::getDeviceSharingSwitch(
    Glib::VariantBase &property,
    [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<bool>::create(m_deviceSharingSwitch);
}

bool Manager::setDeviceSharingSwitch([[maybe_unused]] const Glib::ustring &propertyName,
                                     const Glib::VariantBase &value) noexcept {
    Glib::Variant<bool> v = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(value);
    m_deviceSharingSwitch = v.get();
    m_propertyDeviceSharingSwitch->emitChanged(Glib::Variant<bool>::create(m_deviceSharingSwitch));
    cooperationStatusChanged(m_deviceSharingSwitch);
    return true;
}

bool Manager::hasPcMachinePaired() const {
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;
        if (machine->m_paired && machine->isPcMachine()) {
            return true;
        }
    }

    return false;
}

bool Manager::hasAndroidPaired() const {
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;
        if (machine->m_paired && machine->isAndroid()) {
            return true;
        }
    }

    return false;
}

void Manager::scanAux() noexcept {
    Message base;
    ScanRequest *request = base.mutable_scanrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_uuid);
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DEVICE_OS_LINUX);
    request->set_port(m_port);

    m_socketScan->send(m_scanAddr, MessageHelper::genMessage(base));
}

void Manager::removeInputGrabber(const std::filesystem::path &path) {
    m_async->wake([this, path]() { m_inputGrabbers.erase(path.string()); });
}

bool Manager::tryFlowOut(uint16_t direction, uint16_t x, uint16_t y) {
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;
        if (machine->m_deviceSharing && machine->m_direction == direction) {
            machine->flowTo(direction, x, y);
            onFlowOut(machine);
            return true;
        }
    }

    return false;
}

void Manager::cooperationStatusChanged(bool enable) {
    if (enable) {
        scanAux();
    } else {
        for (const auto &v : m_machines) {
            const std::shared_ptr<Machine> &machine = v.second;
            if (machine->m_paired) {
                machine->m_conn->close();
            }
        }
        m_machines.clear();
        m_propertyMachines->emitChanged(
            Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
    }
}

void Manager::updateMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo) {
    auto iter = m_machines.find(devInfo.uuid());
    if (iter == m_machines.end()) {
        addMachine(ip, port, devInfo);
    } else {
        iter->second->receivedPing();

        // existed need update machine info in case of the request pc restarted
        iter->second->updateMachineInfo(ip, port, devInfo);
    }
}

void Manager::addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo) {
    auto shortUUID = devInfo.uuid();
    shortUUID.resize(8);
    fs::path dataPath = m_dataDir / shortUUID;

    if (devInfo.os() == DEVICE_OS_ANDROID) {
        auto m = std::make_shared<AndroidMachine>(this,
                                                  m_clipboard.get(),
                                                  m_uvLoop,
                                                  m_service,
                                                  m_lastMachineIndex,
                                                  dataPath,
                                                  ip,
                                                  port,
                                                  devInfo);
        m_machines.insert(std::pair(devInfo.uuid(), m));
    } else {
        auto m = std::make_shared<PCMachine>(this,
                                             m_clipboard.get(),
                                             m_uvLoop,
                                             m_service,
                                             m_lastMachineIndex,
                                             dataPath,
                                             ip,
                                             port,
                                             devInfo);
        m_machines.insert(std::pair(devInfo.uuid(), m));
    }
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

    if (!m_deviceSharingSwitch) {
        return;
    }

    auto buff = data.get();
    auto &header = MessageHelper::parseMessageHeader(buff);
    if (!header.legal()) {
        spdlog::error("illegal message from {}", addr->toString());
        return;
    }

    buff += header_size;
    size -= header_size;

    auto base = MessageHelper::parseMessageBody<Message>(buff, size);
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
        auto uuid = request.deviceinfo().uuid();
        if (uuid == m_uuid) {
            return;
        }

        if (!isValidUUID(uuid)) {
            return;
        }

        updateMachine(ipv4->ip(), request.port(), request.deviceinfo());

        Message msg;
        ScanResponse *response = msg.mutable_scanresponse();
        response->set_key(SCAN_KEY);
        response->mutable_deviceinfo()->set_uuid(m_uuid);
        response->mutable_deviceinfo()->set_name(Net::getHostname());
        response->mutable_deviceinfo()->set_os(DEVICE_OS_LINUX);
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

        auto uuid = resp.deviceinfo().uuid();
        if (!isValidUUID(uuid)) {
            return;
        }

        updateMachine(addr->ipv4()->ip(), resp.port(), resp.deviceinfo());

        spdlog::info("{} responded", resp.deviceinfo().name());
        break;
    }
    case Message::PayloadCase::kServiceStoppedNotification: {
        const auto &notification = base.servicestoppednotification();
        const auto& uuid = notification.deviceuuid();

        auto iter = m_machines.find(uuid);
        if (iter != m_machines.end()) {
            if (iter->second->m_paired) {
                iter->second->m_conn->close();
            }

            m_machines.erase(uuid);
            m_propertyMachines->emitChanged(
                Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
        }
        break;
    }
    default: {
        spdlog::error("unknown data type");
        break;
    }
    }
}

void Manager::handleNewConnection(bool) noexcept {
    spdlog::debug("new connection received");

    auto socketConnected = m_listenPair->accept();
    socketConnected->onReceived([this, socketConnected](uvxx::Buffer &buff) {
        auto res = MessageHelper::parseMessage<Message>(buff);
        if (!res.has_value()) {
            if (res.error() == MessageHelper::PARSE_ERROR::ILLEGAL_MESSAGE) {
                spdlog::error("illegal message from {}, close the connection",
                              socketConnected->remoteAddress()->toString());
                socketConnected->close();
                socketConnected->onReceived(nullptr);
            }
            return;
        }

        Message &msg = res.value();

        const auto &request = msg.pairrequest();
        if (request.key() != SCAN_KEY) {
            spdlog::error("key mismatch {}", SCAN_KEY);
            socketConnected->close();
            socketConnected->onReceived(nullptr);
            return;
        }

        auto i = m_machines.find(request.deviceinfo().uuid());
        if (i == m_machines.end()) {
            // TODO: return failed
            spdlog::error("cannot found device with uuid {}", request.deviceinfo().uuid());
            socketConnected->close();
            socketConnected->onReceived(nullptr);
            return;
        }

        auto machine = i->second;
        if ((machine->isPcMachine() && hasPcMachinePaired())
            || (machine->isAndroid() && hasAndroidPaired())) {
            // TODO tips
            spdlog::error("cannot pair this device, this machine is paired with other machine");
            socketConnected->close();
            socketConnected->onReceived(nullptr);
            return;
        }

        auto remote = socketConnected->remoteAddress();
        spdlog::info("connected by {}@{}", request.deviceinfo().name().c_str(), remote->toString());

        machine->onPair(socketConnected);
    });
    socketConnected->startRead();
}

void Manager::ping(const std::string &ip, uint16_t port) {
    Message msg;
    ScanRequest *request = msg.mutable_scanrequest();
    request->set_key(SCAN_KEY);
    request->mutable_deviceinfo()->set_uuid(m_uuid);
    request->mutable_deviceinfo()->set_name(Net::getHostname());
    request->mutable_deviceinfo()->set_os(DEVICE_OS_LINUX);
    request->set_port(m_port);

    m_socketScan->send(uvxx::IPv4Addr::create(ip, port), MessageHelper::genMessage(msg));
}

void Manager::onMachineOffline(const std::string &uuid) {
    m_machines.erase(m_machines.find(uuid));

    m_propertyMachines->emitChanged(
        Glib::Variant<std::vector<Glib::DBusObjectPathString>>::create(getMachinePaths()));
}

void Manager::onStartDeviceSharing(const std::weak_ptr<Machine> &machine, bool proactively) {
    if (proactively) {
        m_displayServer->startEdgeDetection();
    } else {
        m_displayServer->hideMouse(true);
        onFlowOut(machine);
    }

    m_deviceSharingCnt++;
    inhibitScreensaver();
}

void Manager::onStopDeviceSharing() {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber.second->stop();
    }

    m_displayServer->stopEdgeDetection();
    m_displayServer->hideMouse(false);

    m_deviceSharingCnt--;
    unInhibitScreensaver();
}

void Manager::onFlowBack(uint16_t direction, uint16_t x, uint16_t y) {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber.second->stop();
    }

    m_displayServer->flowBack(direction, x, y);
}

void Manager::onFlowOut(const std::weak_ptr<Machine> &machine) {
    for (auto &inputGrabbers : m_inputGrabbers) {
        inputGrabbers.second->setMachine(machine);
        inputGrabbers.second->start();
    }
}

void Manager::onClipboardTargetsChanged(const std::vector<std::string> &targets) {
    for (auto &[uuid, machine] : m_machines) {
        if (!machine->m_paired) {
            continue;
        }

        machine->onClipboardTargetsChanged(targets);
    }
}

bool Manager::onReadClipboardContent(const std::string &target) {
    auto machine = m_clipboardOwner.lock();
    if (machine) {
        machine->readTarget(target);
        return true;
    }

    return false;
}

void Manager::onMachineOwnClipboard(const std::weak_ptr<Machine> &machine,
                                    const std::vector<std::string> &targets) {
    m_clipboardOwner = machine;
    m_clipboard->newClipboardOwnerTargets(targets);
}

static const std::string applicationName = "DDE Cooperation";
static const std::string inhibitReason = "cooperating";

void Manager::inhibitScreensaver() {
    if (m_deviceSharingCnt == 0) {
        return;
    }

    if (m_inhibitCookie) {
        return;
    }

    try {
        auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
            Glib::Variant<Glib::ustring>::create(applicationName),
            Glib::Variant<Glib::ustring>::create(inhibitReason),
        });
        auto res = m_powersaverProxy->call_sync("Inhibit", params);
        Glib::Variant<uint32_t> cookie;
        res.get_child(cookie);
        m_inhibitCookie = cookie.get();
    } catch (Glib::Error &e) {
        spdlog::error("failed to inhibit screensaver: {}", std::string(e.what()));
    }
}

void Manager::unInhibitScreensaver() {
    if (m_deviceSharingCnt != 0) {
        return;
    }

    if (!m_inhibitCookie) {
        return;
    }

    try {
        auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
            Glib::Variant<uint32_t>::create(m_inhibitCookie),
        });
        m_powersaverProxy->call_sync("UnInhibit", params);
        m_inhibitCookie = 0;
    } catch (Glib::Error &e) {
        spdlog::error("failed to uninhibit screensaver: {}", std::string(e.what()));
    }
}

void Manager::sendServiceStoppedNotification() const {
    for (const auto &v : m_machines) {
        const std::shared_ptr<Machine> &machine = v.second;

        Message base;
        ServiceStoppedNotification *notification = base.mutable_servicestoppednotification();
        notification->set_deviceuuid(machine->m_uuid);

        m_socketScan->send(uvxx::IPv4Addr::create(machine->ip(),m_scanPort), MessageHelper::genMessage(base));
    }
}
