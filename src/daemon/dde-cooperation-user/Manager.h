#ifndef DDE_COOPERATION_USER_AGENT_H
#define DDE_COOPERATION_USER_AGENT_H

#include <memory>
#include <thread>

#include <glibmm.h>

#include "dbus/dbus.h"
#include "DisplayServer.h"

class Manager {
public:
    Manager();
    ~Manager() = default;

    bool onFlow(uint16_t direction, uint16_t x, uint16_t y);

protected:
    void startCooperation(const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void flowBack(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void newRequest(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

private:
    Glib::RefPtr<Gio::DBus::Connection> m_conn;
    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodStartCooperation;
    Glib::RefPtr<DBus::Method> m_methodFlowBack;

    Glib::RefPtr<Gio::DBus::Proxy> m_serviceProxy;

    std::unique_ptr<DisplayServer> m_displayServer;

    std::thread m_edgeDetectorThread;

    void newCooperationReq(Glib::RefPtr<Gio::DBus::Proxy> req);
};

#endif // !DDE_COOPERATION_USER_AGENT_H
