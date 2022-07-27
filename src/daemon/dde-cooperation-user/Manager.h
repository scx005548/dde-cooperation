#ifndef DDE_COOPERATION_USER_AGENT_H
#define DDE_COOPERATION_USER_AGENT_H

#include <memory>
#include <unordered_map>
#include <thread>

#include <glibmm.h>

#include "dbus/dbus.h"
#include "DisplayServer.h"
#include "FuseServer.h"
#include "FuseClient.h"

class Manager {
public:
    Manager();
    ~Manager() = default;

    bool onFlow(uint16_t direction, uint16_t x, uint16_t y);

protected:
    void startCooperation(const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void flowBack(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void newRequest(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void mountFuse(const Glib::VariantContainerBase &args,
                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

private:
    std::filesystem::path m_mountpoint;

    Glib::RefPtr<Gio::DBus::Connection> m_conn;
    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodStartCooperation;
    Glib::RefPtr<DBus::Method> m_methodFlowBack;
    Glib::RefPtr<DBus::Method> m_methodNewRequest;
    Glib::RefPtr<DBus::Method> m_methodMountFuse;

    Glib::RefPtr<Gio::DBus::Proxy> m_serviceProxy;

    std::unique_ptr<DisplayServer> m_displayServer;

    std::unordered_map<std::string, std::unique_ptr<FuseServer>> m_fuseServers;
    std::unordered_map<std::string, std::unique_ptr<FuseClient>> m_fuseClients;

    std::thread m_displayServerThread;

    // TODO: youhua
    std::thread m_copyThread;

    std::vector<std::string> m_incommingSendFiles;

    std::filesystem::path getMountpoint(std::string uuid);

    void newCooperationReq(Glib::RefPtr<Gio::DBus::Proxy> req);
    void newSendFileReq(Glib::RefPtr<Gio::DBus::Proxy> req);
    void newFilesystemServer(Glib::RefPtr<Gio::DBus::Proxy> req);
};

#endif // !DDE_COOPERATION_USER_AGENT_H
