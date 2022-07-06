#ifndef DDE_COOPERATION_DAEMON_DEVICE_H
#define DDE_COOPERATION_DAEMON_DEVICE_H

#include <glibmm.h>

#include "dbus/dbus.h"

#include "protocol/pair.pb.h"
#include "protocol/cooperation.pb.h"

namespace uvxx {
class Loop;
class Async;
class TCP;
} // namespace uvxx

class Cooperation;
class InputEventRequest;
class Request;

class Machine : public std::enable_shared_from_this<Machine> {
public:
    Machine(Cooperation *cooperation,
            const std::shared_ptr<uvxx::Loop> &loop,
            Glib::RefPtr<DBus::Service> service,
            uint32_t id,
            const Glib::ustring &ip,
            uint16_t port,
            const DeviceInfo &sp);
    ~Machine();

    Glib::ustring path() const { return m_path; }

    void onPair(const std::shared_ptr<uvxx::TCP> &sock);
    void setCooperationRequest(const std::shared_ptr<Request> &req);

    const Glib::ustring &ip() const { return m_ip; };

    void handleInputEvent(const InputEventRequest &event);

protected:
    void pair(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void disconnect(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void sendFile(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void requestCooperate(const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void stopCooperation(const Glib::VariantContainerBase &args,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void flowTo(const Glib::VariantContainerBase &args,
                const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void getIP(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPort(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getUUID(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getName(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPaired(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getOS(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCompositor(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCooperating(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getDirection(Glib::VariantBase &property, const Glib::ustring &propertyName) const;

private:
    Cooperation *m_cooperation;

    const Glib::ustring m_path;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodPair;
    Glib::RefPtr<DBus::Method> m_methodDisconnect;
    Glib::RefPtr<DBus::Method> m_methodSendFile;
    Glib::RefPtr<DBus::Method> m_methodRequestCooperate;
    Glib::RefPtr<DBus::Method> m_methodStopCooperation;
    Glib::RefPtr<DBus::Method> m_methodFlowTo;

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

    bool m_cooperating;
    Glib::RefPtr<DBus::Property> m_propertyCooperating;

    uint16_t m_direction;
    Glib::RefPtr<DBus::Property> m_propertyDirection;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
    std::shared_ptr<uvxx::TCP> m_conn;

    std::shared_ptr<Request> m_cooperationRequest;

    void handleDisconnected();
    void dispatcher(std::shared_ptr<char[]> buffer, ssize_t size) noexcept;
    void handlePairResponse(const PairResponse &resp);
    void handleCooperateRequest();
    void handleCooperateRequestAccepted();
    void handleStopCooperation();
    void handleAcceptCooperation(bool accepted,
                                 const std::map<Glib::ustring, Glib::VariantBase> &hint,
                                 uint32_t serial);

    void stopCooperationAux();
};

#endif // !DDE_COOPERATION_DAEMON_DEVICE_H
