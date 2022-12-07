#ifndef MACHINE_PCMACHINE_H
#define MACHINE_PCMACHINE_H

#include "Machine.h"

class PCMachine : public Machine {
public:
    PCMachine(Manager *manager,
              ClipboardBase *clipboard,
              QDBusConnection service,
              uint32_t id,
              const std::filesystem::path &dataDir,
              const std::string &ip,
              uint16_t port,
              const DeviceInfo &sp);

    virtual void handleConnected() override;
    virtual void handleDisconnected() override;

private:
    void mountFs(const std::string &path);
};

#endif // !MACHINE_PCMACHINE_H
