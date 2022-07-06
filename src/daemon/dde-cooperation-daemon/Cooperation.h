#ifndef DDE_COOPERATION_DAEMON_COOPERATION_H
#define DDE_COOPERATION_DAEMON_COOPERATION_H

#include <map>
#include <filesystem>
#include <memory>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include "KeyPair.h"
#include "InputDevice.h"
#include "InputEvent.h"
#include "dbus/dbus.h"
#include "utils/net.h"

#include "protocol/pair.pb.h"
#include "protocol/cooperation.pb.h"

#include "uvxx/noncopyable.h"

namespace uvxx {
class Loop;
class Async;
class Addr;
class TCP;
class UDP;
} // namespace uvxx

class Machine;

class Cooperation : public noncopyable {
public:
    Cooperation();
    ~Cooperation();

    std::string uuid() const noexcept { return m_uuid; }

    void handleReceivedCooperateRequest(Machine *machine);
    void handleStartCooperation(const std::weak_ptr<Machine> &machine);
    void handleRemoteAcceptedCooperation();
    bool handleReceivedInputEventRequest(const InputEventRequest &event);
    void handleStopCooperation();
    void handleFlowBack(uint16_t direction, uint16_t x, uint16_t y);
    void handleFlowOut(std::weak_ptr<Machine> machine);

protected:
    // DBus method handlers
    void scan(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void knock(const Glib::VariantContainerBase &args,
               const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void registerUserDeamon(const Glib::VariantContainerBase &args,
                            const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    // DBus property handlers
    void getMachines(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;
    void getEnableCooperation(Glib::VariantBase &property,
                              const Glib::ustring &propertyName) const noexcept;
    bool setEnableCooperation(const Glib::ustring &propertyName,
                              const Glib::VariantBase &value) noexcept;

private:
    std::thread m_uvThread;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
    std::shared_ptr<uvxx::UDP> m_socketScan;
    uint16_t m_port;
    std::shared_ptr<uvxx::TCP> m_listenPair;

    static const std::filesystem::path dataDir;

    uint32_t m_lastRequestId;

    std::string m_uuid;

    Glib::RefPtr<Gio::DBus::Connection> m_bus;
    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    // DBus methods
    Glib::RefPtr<DBus::Method> m_methodScan;
    Glib::RefPtr<DBus::Method> m_methodKnock;
    Glib::RefPtr<DBus::Method> m_methodRegisterUserDeamon;

    // DBus properties
    Glib::RefPtr<DBus::Property> m_propertyMachines;
    std::map<std::string, std::shared_ptr<Machine>> m_machines;
    uint32_t m_lastMachineIndex;

    Glib::RefPtr<DBus::Property> m_propertyEnableCooperation;
    bool m_enableCooperation;

    Glib::RefPtr<Gio::DBus::Proxy> m_dbusProxy;

    Glib::ustring m_userServiceSender;
    Glib::RefPtr<Gio::DBus::Proxy> m_userServiceProxy;

    const uint16_t m_scanPort = 51595;
    std::shared_ptr<uvxx::Addr> m_scanAddr;

    KeyPair m_keypair;

    std::unordered_map<std::string, std::unique_ptr<InputDevice>> m_inputDevices;
    std::unordered_map<DeviceType, std::unique_ptr<InputEvent>> m_inputEvents;

    void ensureDataDirExists();
    void initUUID();

    void addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);
    std::vector<Glib::DBusObjectPathString> getMachinePaths() const noexcept;

    void handleSocketError(const std::string &title, const std::string &msg);
    void handleReceivedSocketScan(std::shared_ptr<uvxx::Addr> addr,
                                  std::shared_ptr<char[]> data,
                                  size_t size,
                                  bool partial) noexcept;
    void handleNewConnection(bool) noexcept;

    void handleDBusServiceSignal(const Glib::ustring &sender,
                                 const Glib::ustring &signal,
                                 const Glib::VariantContainerBase &value);
};

#endif // !DDE_COOPERATION_DAEMON_COOPERATION_H
