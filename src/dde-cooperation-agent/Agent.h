#ifndef DDE_COOPERATION_AGENT_AGENT_H
#define DDE_COOPERATION_AGENT_AGENT_H

#include <memory>
#include <glibmm.h>

#include "dbus/dbus.h"
#include "EdgeDetector.h"

class Agent {
public:
    Agent();

protected:
    void scan(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

private:
    Glib::RefPtr<Gio::DBus::Connection> m_conn;
    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;
    Glib::RefPtr<DBus::Method> m_methodScan;

    std::unique_ptr<EdgeDetector> m_edgeDetector;
};

#endif // !DDE_COOPERATION_AGENT_AGENT_H
