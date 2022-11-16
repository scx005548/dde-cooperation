#ifndef DDE_COOPERATION_DAEMON_MANAGER_H
#define DDE_COOPERATION_DAEMON_MANAGER_H

#include <unordered_map>
#include <filesystem>
#include <memory>
#include <thread>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include "KeyPair.h"
#include "InputGrabberWrapper.h"
#include "dbus/dbus.h"
#include "utils/net.h"
#include "ClipboardBase.h"

#include "protocol/pair.pb.h"
#include "protocol/device_sharing.pb.h"

#include "uvxx/noncopyable.h"

#include <DConfig>

DCORE_USE_NAMESPACE

namespace uvxx {
class Loop;
class Async;
class Addr;
class TCP;
class UDP;
class Pipe;
class Process;
} // namespace uvxx

class FsSendFileRequest;
class FsRequest;
class FsResponse;
class Machine;
class DisplayBase;
class ClipboardBase;

class Manager : public ClipboardObserver, public noncopyable {
public:
    Manager(const std::shared_ptr<uvxx::Loop> &uvLoop, const std::filesystem::path &dataDir);
    ~Manager();

    std::string uuid() const noexcept { return m_uuid; }
    std::string fileStoragePath() const noexcept { return m_fileStoragePath; }
    bool tryFlowOut(uint16_t direction, uint16_t x, uint16_t y);
    bool hasPcMachinePaired() const;
    bool hasAndroidPaired() const;
    void removeInputGrabber(const std::filesystem::path &path);

    void ping(const std::string &ip, uint16_t port = m_scanPort);
    void onMachineOffline(const std::string &uuid);
    void onStartDeviceSharing(const std::weak_ptr<Machine> &machine, bool proactively);
    void onStopDeviceSharing();
    void onFlowBack(uint16_t direction, uint16_t x, uint16_t y);
    void onFlowOut(const std::weak_ptr<Machine> &machine);
    virtual void onClipboardTargetsChanged(const std::vector<std::string> &targets) override;
    virtual bool onReadClipboardContent(const std::string &target) override;
    void onMachineOwnClipboard(const std::weak_ptr<Machine> &machine,
                               const std::vector<std::string> &targets);
    void onInputEvent();

protected:
    // DBus method handlers
    void getUUID(const Glib::VariantContainerBase &args,
                 const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void scan(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void knock(const Glib::VariantContainerBase &args,
               const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void sendFile(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void setFileStoragePath(const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    // DBus property handlers
    void getMachines(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;
    void getDeviceSharingSwitch(Glib::VariantBase &property,
                                const Glib::ustring &propertyName) const noexcept;
    bool setDeviceSharingSwitch(const Glib::ustring &propertyName,
                                const Glib::VariantBase &value) noexcept;
    void getFileStoragePath(Glib::VariantBase &property, const Glib::ustring &propertyName) const noexcept;

private:
    const std::filesystem::path m_dataDir;
    const std::filesystem::path m_mountRoot;

    uint32_t m_lastMachineIndex;
    std::unordered_map<std::string, std::shared_ptr<Machine>> m_machines;
    bool m_deviceSharingSwitch;
    std::weak_ptr<Machine> m_clipboardOwner;
    std::unique_ptr<DisplayBase> m_displayServer;
    std::unique_ptr<ClipboardBase> m_clipboard;
    uint32_t m_lastRequestId;
    std::unordered_map<std::string, std::shared_ptr<InputGrabberWrapper>> m_inputGrabbers;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
    std::shared_ptr<uvxx::UDP> m_socketScan;
    uint16_t m_port;
    std::shared_ptr<uvxx::TCP> m_listenPair;
    static const uint16_t m_scanPort = 51595;
    std::shared_ptr<uvxx::Addr> m_scanAddr;

    std::string m_uuid;
    Glib::ustring m_fileStoragePath;

    Glib::RefPtr<Gio::DBus::Connection> m_bus;
    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    // DBus methods
    Glib::RefPtr<DBus::Method> m_methodGetUUID;
    Glib::RefPtr<DBus::Method> m_methodScan;
    Glib::RefPtr<DBus::Method> m_methodKnock;
    Glib::RefPtr<DBus::Method> m_methodSendFile;
    Glib::RefPtr<DBus::Method> m_methodSetFileStoragePath;

    // DBus properties
    Glib::RefPtr<DBus::Property> m_propertyMachines;
    Glib::RefPtr<DBus::Property> m_propertyDeviceSharingSwitch;
    Glib::RefPtr<DBus::Property> m_propertyFileStoragePath;

    Glib::RefPtr<Gio::DBus::Proxy> m_dbusProxy;
    Glib::RefPtr<Gio::DBus::Proxy> m_powersaverProxy;
    int m_deviceSharingCnt;
    uint32_t m_inhibitCookie;

    KeyPair m_keypair;

    std::shared_ptr<DConfig> m_dConfig;

    void scanAux() noexcept;

    void ensureDataDirExists();
    void initUUID();
    std::string newUUID() const;
    bool isValidUUID(const std::string &str) const noexcept;

    void initFileStoragePath();

    void cooperationStatusChanged(bool enable);
    void updateMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);
    void addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);
    std::vector<Glib::DBusObjectPathString> getMachinePaths() const noexcept;
    void inhibitScreensaver();
    void unInhibitScreensaver();

    void handleSocketError(const std::string &title, const std::string &msg);
    void handleReceivedSocketScan(std::shared_ptr<uvxx::Addr> addr,
                                  std::shared_ptr<char[]> data,
                                  size_t size,
                                  bool partial) noexcept;
    void handleNewConnection(bool) noexcept;
    void sendServiceStoppedNotification() const;
};

#endif // !DDE_COOPERATION_DAEMON_MANAGER_H
