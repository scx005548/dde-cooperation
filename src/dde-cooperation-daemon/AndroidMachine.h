#ifndef DDE_COOPERATION_DAEMON_ANDROIDMACHINE_H
#define DDE_COOPERATION_DAEMON_ANDROIDMACHINE_H

#include "Machine.h"

namespace uvxx {
class Process;
}

class AndroidMachine : public Machine {
public:
    AndroidMachine(Manager *manager,
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

private:
    std::shared_ptr<uvxx::Process> m_process;
};

#endif // !DDE_COOPERATION_DAEMON_ANDROIDMACHINE_H
