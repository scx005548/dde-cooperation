#ifndef MACHINE_ANDROIDMACHINE_H
#define MACHINE_ANDROIDMACHINE_H

#include "Machine.h"

class AndroidMachineDBusAdaptor;
class AndroidMainWindow;
class SendTransfer;

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

public:
    void startCast();

protected:
    virtual void handleConnected() override;
    virtual void handleDisconnected() override;
    virtual void handleCastRequest(const CastRequest &req) override;
    virtual void sendFiles(const QStringList &filePaths) override;

private:
    AndroidMachineDBusAdaptor *m_dbusAdaptorAndroid;
    AndroidMainWindow *m_mainWindow;
};

#endif // !MACHINE_ANDROIDMACHINE_H
