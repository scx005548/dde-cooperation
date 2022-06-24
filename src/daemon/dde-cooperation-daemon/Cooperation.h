#ifndef DDE_COOPERATION_DAEMON_COOPERATION_H
#define DDE_COOPERATION_DAEMON_COOPERATION_H

#include <map>
#include <filesystem>
#include <memory>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include "Machine.h"
#include "KeyPair.h"
#include "InputDevice.h"
#include "InputEvent.h"
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
    void knock(const Glib::VariantContainerBase &args,
               const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    // DBus property handlers
    void getMachines(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;

private:
    static const std::filesystem::path dataDir;

    std::string m_uuid;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    // DBus methods
    Glib::RefPtr<DBus::Method> m_methodScan;
    Glib::RefPtr<DBus::Method> m_methodKnock;

    // DBus properties
    Glib::RefPtr<DBus::Property> m_propertyMachines;
    std::map<std::string, std::unique_ptr<Machine>> m_machines;
    uint32_t m_lastMachineIndex;

    Glib::RefPtr<Gio::Socket> m_socketScan;
    Glib::RefPtr<Gio::Socket> m_socketListenScan;
    Glib::RefPtr<Gio::Socket> m_socketListenPair;

    const uint16_t m_scanPort = 51595;
    Glib::RefPtr<Gio::SocketAddress> m_scanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenScanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenPairAddr;

    KeyPair m_keypair;

    std::unordered_map<std::string, std::unique_ptr<InputDevice>> m_inputDevices;
    std::unordered_map<DeviceType, std::unique_ptr<InputEvent>> m_inputEvents;

    void ensureDataDirExists();
    void initUUID();

    void addMachine(const Glib::ustring &ip, uint16_t port, const DeviceInfo &devInfo);
    std::vector<Glib::DBusObjectPathString> getMachinePaths() const noexcept;

    bool handleReceivedScanRequest(Glib::IOCondition cond) noexcept;
    bool handleReceivedScanResponse(Glib::IOCondition cond) noexcept;
    bool handleReceivedPairRequest(Glib::IOCondition cond) noexcept;
    bool handleReceivedCooperateRequest(Machine *machine);
    bool handleReceivedInputEventRequest(const InputEventRequest &event);
};

#endif // !DDE_COOPERATION_DAEMON_COOPERATION_H
