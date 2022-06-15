#ifndef DDE_COOPERATION_DEVICE_H
#define DDE_COOPERATION_DEVICE_H

#include <glibmm.h>

#include "dbus/dbus.h"

#include "protocol/pair.pb.h"
#include "protocol/input_event.pb.h"

class Cooperation;

class Machine {
public:
    Machine(Cooperation &cooperation,
            Glib::RefPtr<DBus::Service> service,
            uint32_t id,
            const DeviceInfo &sp);
    ~Machine();

    Glib::ustring path() const { return m_path; }

    void onPair(Glib::RefPtr<Gio::Socket> conn);

    using type_signal_cooperationRequest = sigc::signal<bool(Machine *)>;
    type_signal_cooperationRequest onCooperationRequest() { return m_signal_cooperationRequest; }

    using type_signal_flowRequest = sigc::signal<void()>;
    type_signal_flowRequest flowRequest() { return m_signal_flowRequest; }

    using type_signal_inputEvent = sigc::signal<bool(const InputEventRequest &)>;
    type_signal_inputEvent inputEvent() { return m_signal_inputEvent; }

    void handleInputEvent(const InputEventRequest &event);

protected:
    void pair(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void sendFile(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void getUUID(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getName(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPaired(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getOS(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCompositor(Glib::VariantBase &property, const Glib::ustring &propertyName) const;

private:
    Cooperation &m_cooperation;

    type_signal_cooperationRequest m_signal_cooperationRequest;
    type_signal_flowRequest m_signal_flowRequest;
    type_signal_inputEvent m_signal_inputEvent;

    const Glib::ustring m_path;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodPair;
    Glib::RefPtr<DBus::Method> m_methodSendFile;

    Glib::ustring m_uuid;
    Glib::RefPtr<DBus::Property> m_propertyUUID;

    Glib::ustring m_name;
    Glib::RefPtr<DBus::Property> m_propertyName;

    bool m_paired;
    Glib::RefPtr<DBus::Property> m_propertyPaired;

    DeviceOS m_os;
    Glib::RefPtr<DBus::Property> m_propertyOS;

    Compositor m_compositor;
    Glib::RefPtr<DBus::Property> m_propertyCompositor;

    Glib::RefPtr<Gio::Socket> m_socketConnect;

    bool mainHandler(Glib::IOCondition cond, const Glib::RefPtr<Gio::Socket> &sock) noexcept;
    void handlePairResponse(const PairResponse &resp);
    void handleCooperateRequest();
};

#endif // !DDE_COOPERATION_DEVICE_H
