#ifndef MACHINE_ANDROIDMACHINE_H
#define MACHINE_ANDROIDMACHINE_H

#include "Machine.h"
#include "SendTransfer.h"

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

protected:
    virtual void handleConnected() override;
    virtual void handleDisconnected() override;
    virtual void handleCaseRequest(const CastRequest &req) override;
    virtual void handleTransferResponse(const TransferResponse &resp) override;
    virtual void sendFiles(const QStringList &filePaths) override;

private:
    uint32_t m_currentTransferId;

    std::unordered_map<uint32_t, std::unique_ptr<SendTransfer>> m_sendTransfers;
};

#endif // !MACHINE_ANDROIDMACHINE_H
