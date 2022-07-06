#include "Manager.h"

#include <spdlog/spdlog.h>

#include "DisplayServer/X11.h"

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
    , m_serviceProxy(Gio::DBus::Proxy::Proxy::create_sync(m_conn,
                                                          "com.deepin.Cooperation",
                                                          "/com/deepin/Cooperation",
                                                          "com.deepin.Cooperation")) {
    m_interface->exportMethod(m_methodStartCooperation);
    m_interface->exportMethod(m_methodFlowBack);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    m_serviceProxy->call_sync("RegisterUserDeamon");

    // if (getenv("WAYLAND_DISPLAY")) {
    //     // TODO: wayland
    // } else {
    m_displayServer = std::make_unique<X11>(this);
    // }

    m_edgeDetectorThread = std::thread([this] { m_displayServer->start(); });
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
        machineTmp->get_cached_property(dirV, "direction");

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
    }
}

void Manager::newCooperationReq(Glib::RefPtr<Gio::DBus::Proxy> req) {
    auto accept = Glib::Variant<bool>::create(true);
    auto hint = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>>::create({});
    auto params = Glib::Variant<std::vector<Glib::VariantBase>>::create_tuple({accept, hint});
    req->call_sync("Accept", params);
}
