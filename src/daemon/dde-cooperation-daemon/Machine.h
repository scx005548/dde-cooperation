#ifndef DDE_COOPERATION_DAEMON_DEVICE_H
#define DDE_COOPERATION_DAEMON_DEVICE_H

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
            const Glib::ustring &ip,
            uint16_t port,
            const DeviceInfo &sp);
    ~Machine();

    Glib::ustring path() const { return m_path; }

    void onPair(Glib::RefPtr<Gio::Socket> conn);

    using type_signal_receivedCooperationRequest = sigc::signal<bool(Machine *)>;
    type_signal_receivedCooperationRequest onReceivedCooperationRequest() {
        return m_signal_receivedCooperationRequest;
    }

    using type_signal_receivedFlowRequest = sigc::signal<void()>;
    type_signal_receivedFlowRequest onReceivedFlowRequest() { return m_signal_receivedFlowRequest; }

    using type_signal_receivedInputEvent = sigc::signal<bool(const InputEventRequest &)>;
    type_signal_receivedInputEvent onReceivedInputEvent() { return m_signal_receivedInputEvent; }

    void handleInputEvent(const InputEventRequest &event);

protected:
    void pair(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void sendFile(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void getIP(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPort(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getUUID(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getName(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPaired(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getOS(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCompositor(Glib::VariantBase &property, const Glib::ustring &propertyName) const;

private:
    Cooperation &m_cooperation;

    type_signal_receivedCooperationRequest m_signal_receivedCooperationRequest;
    type_signal_receivedFlowRequest m_signal_receivedFlowRequest;
    type_signal_receivedInputEvent m_signal_receivedInputEvent;

    const Glib::ustring m_path;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodPair;
    Glib::RefPtr<DBus::Method> m_methodSendFile;

    Glib::ustring m_ip;
    Glib::RefPtr<DBus::Property> m_propertyIP;

    uint16_t m_port;
    Glib::RefPtr<DBus::Property> m_propertyPort;

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

#endif // !DDE_COOPERATION_DAEMON_DEVICE_H
