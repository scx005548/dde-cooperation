#ifndef MACHINE_ANDROIDMACHINE_H
#define MACHINE_ANDROIDMACHINE_H

#include "Machine.h"

class AndroidMachine : public Machine {
public:
    AndroidMachine(Manager *manager,
                   ClipboardBase *clipboard,
                   QDBusConnection service,
                   uint32_t id,
                   const std::filesystem::path &dataDir,
                   const std::string &ip,
                   uint16_t port,
                   const DeviceInfo &sp);
    virtual void handleConnected() override;
    virtual void handleDisconnected() override;
};

#endif // !MACHINE_ANDROIDMACHINE_H
