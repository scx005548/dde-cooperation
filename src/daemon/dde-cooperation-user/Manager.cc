#include "Manager.h"

#include <map>
#include <filesystem>

#include <spdlog/spdlog.h>

#include "DisplayServer/X11.h"
#include "utils/net.h"
#include "uvxx/TCP.h"

namespace fs = std::filesystem;

Manager::Manager()
    : m_conn(Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SYSTEM))
    , m_service(new DBus::Service(m_conn))
    , m_object(new DBus::Object("/com/deepin/Cooperation/User"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation.User"))
    , m_methodStartCooperation(
          new DBus::Method("StartCooperation",
                           DBus::Method::warp(this, &Manager::startCooperation),
                           {{"start", "b"}}))
    , m_methodFlowBack(new DBus::Method("FlowBack",
                                        DBus::Method::warp(this, &Manager::flowBack),
                                        {{"direction", "q"}, {"x", "q"}, {"y", "q"}}))
    , m_methodNewRequest(new DBus::Method("NewRequest",
                                          DBus::Method::warp(this, &Manager::newRequest),
                                          {{"path", "o"}}))
    , m_methodMountFuse(new DBus::Method("MountFuse",
                                         DBus::Method::warp(this, &Manager::mountFuse),
                                         {{"machine", "o"}, {"ip", "s"}, {"port", "q"}}))
    , m_serviceProxy(Gio::DBus::Proxy::Proxy::create_sync(m_conn,
                                                          "com.deepin.Cooperation",
                                                          "/com/deepin/Cooperation",
                                                          "com.deepin.Cooperation")) {
    m_interface->exportMethod(m_methodStartCooperation);
    m_interface->exportMethod(m_methodFlowBack);
    m_interface->exportMethod(m_methodNewRequest);
    m_interface->exportMethod(m_methodMountFuse);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    std::string runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (runtimeDir.empty()) {
        // TODO: default dir
    }

    fs::path mountpoint = runtimeDir;
    mountpoint /= "dde-cooperation";
    m_mountpoint = mountpoint;
    spdlog::info("mountpoint: {}", m_mountpoint.string());
    if (!fs::exists(m_mountpoint)) {
        fs::create_directories(m_mountpoint);
    }

    m_serviceProxy->call_sync("RegisterUserDeamon");

    // if (getenv("WAYLAND_DISPLAY")) {
    //     // TODO: wayland
    // } else {
    m_displayServer = std::make_unique<X11>(this);
    // }

    m_displayServerThread = std::thread([this] { m_displayServer->start(); });
}

bool Manager::onFlow(uint16_t direction, uint16_t x, uint16_t y) {
    Glib::Variant<std::vector<Glib::DBusObjectPathString>> machinePathsV;
    m_serviceProxy->get_cached_property(machinePathsV, "Machines");

    auto machinePaths = machinePathsV.get();

    // TODO: loop
    Glib::RefPtr<Gio::DBus::Proxy> machine;
    for (auto &path : machinePaths) {
        auto machineTmp = Gio::DBus::Proxy::Proxy::create_sync(m_conn,
                                                               "com.deepin.Cooperation",
                                                               path,
                                                               "com.deepin.Cooperation.Machine");
        Glib::Variant<uint16_t> dirV;
        machineTmp->get_cached_property(dirV, "Direction");

        uint16_t dir = dirV.get();
        if (dir == direction) {
            machine = machineTmp;
            break;
        }
    }
    if (!machine) {
        return false;
    }

    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({
        Glib::Variant<uint16_t>::create(direction),
        Glib::Variant<uint16_t>::create(x),
        Glib::Variant<uint16_t>::create(y),
    });
    machine->call_sync("FlowTo", params);

    return true;
}

void Manager::startCooperation(
    [[maybe_unused]] const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<bool> start;
    args.get_child(start, 0);

    m_displayServer->startEdgeDetection(start.get());

    invocation->return_value(Glib::VariantContainerBase{});
}

void Manager::flowBack(const Glib::VariantContainerBase &args,
                       const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<uint16_t> direction;
    args.get_child(direction, 0);
    Glib::Variant<uint16_t> x;
    args.get_child(x, 1);
    Glib::Variant<uint16_t> y;
    args.get_child(y, 2);

    m_displayServer->flowBack(direction.get(), x.get(), y.get());

    invocation->return_value(Glib::VariantContainerBase{});
}

void Manager::newRequest(const Glib::VariantContainerBase &args,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::DBusObjectPathString> path;
    args.get_child(path, 0);

    invocation->return_value(Glib::VariantContainerBase{});

    spdlog::info("new request: {}", std::string(path.get()));

    auto req = Gio::DBus::Proxy::Proxy::create_sync(m_service->conn(),
                                                    "com.deepin.Cooperation",
                                                    path.get(),
                                                    "com.deepin.Cooperation.Request");
    Glib::Variant<uint16_t> typeV;
    req->get_cached_property(typeV, "Type");
    switch (typeV.get()) {
    case 0: {
        newCooperationReq(req);
        break;
    }
    case 1: {
        newFilesystemServer(req);
        break;
    }
    case 2: {
        newSendFileReq(req);
        break;
    }
    }
}

static void copyToDesktop(fs::path file) {
    fs::path home = getenv("HOME");
    fs::path desktop = home / "Desktop";
    fs::path dst = desktop / file.filename();

    // bool res = fs::copy_file(file, dst);
    auto command = fmt::format("cp {} {}", file.string(), dst.string());
    int res = system(command.c_str());
    spdlog::info("command {}: {}", command, res);
}

void Manager::mountFuse(const Glib::VariantContainerBase &args,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    Glib::Variant<Glib::DBusObjectPathString> machinePath;
    Glib::Variant<Glib::ustring> ip;
    Glib::Variant<uint16_t> port;
    args.get_child(machinePath, 0);
    args.get_child(ip, 1);
    args.get_child(port, 2);

    invocation->return_value(Glib::VariantContainerBase{});

    auto machine = Gio::DBus::Proxy::Proxy::create_sync(m_service->conn(),
                                                        "com.deepin.Cooperation",
                                                        machinePath.get(),
                                                        "com.deepin.Cooperation.Machine");
    Glib::Variant<Glib::ustring> uuidV;
    machine->get_cached_property(uuidV, "UUID");

    std::string uuid = std::string(uuidV.get());

    fs::path mountpoint = getMountpoint(uuid);
    auto client = std::make_unique<FuseClient>(std::string(ip.get()), port.get(), mountpoint);
    m_fuseClients.emplace(std::pair{uuid, std::move(client)});

    spdlog::info("mountpoint: {}", mountpoint.string());

    m_copyThread = std::thread([this, mountpoint]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);

        // TODO: youhua
        for (const std::string &file : m_incommingSendFiles) {
            auto path = mountpoint.string();
            path += file;
            spdlog::info("mp==={}, path======{}", mountpoint.string(), path);
            copyToDesktop(fs::path(path));
        }

        m_incommingSendFiles.clear();
    });
}

std::filesystem::path Manager::getMountpoint(std::string uuid) {
    uuid.resize(8);
    return m_mountpoint / uuid;
}

void Manager::newCooperationReq(Glib::RefPtr<Gio::DBus::Proxy> req) {
    auto accept = Glib::Variant<bool>::create(true);
    auto hint = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create({});
    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({accept, hint});
    req->call_sync("Accept", params);
}

void Manager::newSendFileReq(Glib::RefPtr<Gio::DBus::Proxy> req) {
    auto sendfile = Gio::DBus::Proxy::Proxy::create_sync(m_service->conn(),
                                                         "com.deepin.Cooperation",
                                                         req->get_object_path(),
                                                         "com.deepin.Cooperation.Request.SendFile");
    Glib::Variant<Glib::ustring> pathV;
    sendfile->get_cached_property(pathV, "Path");
    std::string path = std::string(pathV.get());
    if (m_fuseClients.empty()) {
        m_incommingSendFiles.push_back(path);
    } else {
        auto mp = getMountpoint(m_fuseClients.begin()->first);
        auto file = std::string(mp);
        file += path;
        copyToDesktop(fs::path(file));
    }

    auto accept = Glib::Variant<bool>::create(true);
    auto hint = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create({});
    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({accept, hint});
    req->call_sync("Accept", params);
}

void Manager::newFilesystemServer(Glib::RefPtr<Gio::DBus::Proxy> req) {
    try {
        fs::path p{"/"};

        auto server = std::make_unique<FuseServer>(p);
        auto port = server->port();

        m_fuseServers.emplace(std::pair{p.string(), std::move(server)});

        auto accept = Glib::Variant<bool>::create(true);
        auto hint = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create(
            std::map<Glib::ustring, Glib::VariantBase>{
                std::pair{"port", Glib::Variant<uint16_t>::create(port)}});
        auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({accept, hint});
        req->call_sync("Accept", params);
    } catch (Glib::Error &e) {
        spdlog::error("newFilesystemServer: {}", std::string(e.what()));
    }
}
