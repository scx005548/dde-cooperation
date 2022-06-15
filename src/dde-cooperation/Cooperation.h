#ifndef DDE_COOPERATION_COOPERATION_H
#define DDE_COOPERATION_COOPERATION_H

#include <map>
#include <filesystem>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include "Device.h"
#include "KeyPair.h"
#include "dbus/dbus.h"
#include "utils/net.h"

#include "protocol/pair.pb.h"

class Cooperation {
public:
    Cooperation();

    std::string uuid() const noexcept { return m_uuid; }

protected:
    // DBus method handlers
    void scan(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    // DBus property handlers
    void getDevices(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;

private:
    static const std::filesystem::path dataDir;

    std::string m_uuid;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    // DBus methods
    Glib::RefPtr<DBus::Method> m_methodScan;

    // DBus properties
    Glib::RefPtr<DBus::Property> m_propertyDevices;
    std::map<std::string, Device> m_devices;
    uint32_t m_lastDeviceIndex;

    Glib::RefPtr<Gio::Socket> m_socketScan;
    Glib::RefPtr<Gio::Socket> m_socketListenScan;
    Glib::RefPtr<Gio::Socket> m_socketListenPair;

    const uint16_t m_scanPort = 51595;
    Glib::RefPtr<Gio::SocketAddress> m_scanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenScanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenPairAddr;

    KeyPair m_keypair;

    void ensureDataDirExists();
    void initUUID();

    bool m_scanRequestHandler(Glib::IOCondition cond) const noexcept;
    bool m_scanResponseHandler(Glib::IOCondition cond) noexcept;

    bool m_pairRequestHandler(Glib::IOCondition cond) noexcept;
};

#endif // !DDE_COOPERATION_COOPERATION_H
