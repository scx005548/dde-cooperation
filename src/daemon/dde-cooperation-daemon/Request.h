#ifndef DDE_COOPERATION_DAEMON_REQUEST_H
#define DDE_COOPERATION_DAEMON_REQUEST_H

#include "dbus/dbus.h"

class Request {
public:
    enum class Type {
        Cooperation,
        Fuse,
    };

    Request(const Glib::RefPtr<DBus::Service> &service, uint32_t id, Type type, uint32_t serial);
    ~Request();

    Glib::DBusObjectPathString path() const;

    using type_signal_onAccept = sigc::signal<
        void(bool, std::map<Glib::ustring, Glib::VariantBase>, uint32_t)>;
    type_signal_onAccept onAccept() { return m_signal_onAccept; }

protected:
    // DBus method handlers
    void accept(const Glib::VariantContainerBase &args,
                const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    // DBus property handlers
    void getType(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;
    void getMachine(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;

private:
    uint32_t m_serial;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodAccept;
    Type m_type;
    Glib::RefPtr<DBus::Property> m_propertyType;
    Glib::DBusObjectPathString m_machinePath;
    Glib::RefPtr<DBus::Property> m_propertyMachine;

    type_signal_onAccept m_signal_onAccept;
};

#endif // !DDE_COOPERATION_DAEMON_REQUEST_H
