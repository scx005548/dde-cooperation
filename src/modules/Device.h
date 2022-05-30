#ifndef DEVICE_H
#define DEVICE_H

#include <glibmm.h>

#include "dbus/dbus.h"
#include "FileSender.h"
#include "FileReceiver.h"

#include "protocol/pair.pb.h"

class Cooperation;

class Device {
public:
    Device(Cooperation &cooperation,
           Glib::RefPtr<DBus::Service> service,
           uint32_t id,
           const DeviceInfo &sp);
    ~Device();

    Glib::ustring path() const { return m_path; }

    void onPair(Glib::RefPtr<Gio::Socket> conn);

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

    FileSender m_fileSender;
    FileReceiver m_fileReciever;

    bool mainHandler(Glib::IOCondition cond, const Glib::RefPtr<Gio::Socket> &sock) noexcept;
    void handlePairRespinse(const PairResponse &resp);
};

#endif // !DEVICE_H
