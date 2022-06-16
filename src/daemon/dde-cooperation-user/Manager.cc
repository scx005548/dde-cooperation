#include "Manager.h"

#include <spdlog/spdlog.h>

#include "EdgeDetector/X11.h"

Manager::Manager()
    : m_conn(Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SYSTEM))
    , m_service(new DBus::Service(m_conn))
    , m_object(new DBus::Object("/com/deepin/Cooperation/User"))
    , m_interface(new DBus::Interface("com.deepin.Cooperation.User"))
    , m_methodScan(new DBus::Method("Scan", DBus::Method::warp(this, &Manager::scan))) {
    m_interface->exportMethod(m_methodScan);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);

    if (getenv("WAYLAND_DISPLAY")) {
        // TODO: wayland
    } else {
        m_edgeDetector = std::make_unique<X11>();
    }
    m_edgeDetector->start();
}

void Manager::scan([[maybe_unused]] const Glib::VariantContainerBase &args,
                 const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    invocation->return_value(Glib::VariantContainerBase{});
}
