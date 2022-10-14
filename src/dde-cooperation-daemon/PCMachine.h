#ifndef DDE_COOPERATION_DAEMON_PCMACHINE_H
#define DDE_COOPERATION_DAEMON_PCMACHINE_H

#include "Machine.h"

#include "dbus/dbus.h"

class PCMachine : public Machine {
public:
    PCMachine(Manager *manager,
              ClipboardBase *clipboard,
              const std::shared_ptr<uvxx::Loop> &uvLoop,
              Glib::RefPtr<DBus::Service> service,
              uint32_t id,
              const std::filesystem::path &dataDir,
              const Glib::ustring &ip,
              uint16_t port,
              const DeviceInfo &sp);

    virtual void handleConnected() override;
    virtual void handleDisconnected() override;

protected:
    void mountFs(const Glib::VariantContainerBase &args,
                 const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

private:
    Glib::RefPtr<DBus::Interface> m_linuxInterface;

    Glib::RefPtr<DBus::Method> m_methodMountFs;

    void mountFs(const std::string &path);
};

#endif // !DDE_COOPERATION_DAEMON_PCMACHINE_H
