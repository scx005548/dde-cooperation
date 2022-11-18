#include "AndroidMachine.h"

#include <filesystem>
#include <algorithm>

#include "Manager.h"
#include "Android/AndroidMainWindow.h"

AndroidMachine::AndroidMachine(Manager *manager,
                               ClipboardBase *clipboard,
                               QDBusConnection service,
                               uint32_t id,
                               const std::filesystem::path &dataDir,
                               const std::string &ip,
                               uint16_t port,
                               const DeviceInfo &sp)
    : Machine(manager, clipboard, service, id, dataDir, ip, port, sp)
    , m_currentTransferId(0) {
}

void AndroidMachine::handleConnected() {
}

void AndroidMachine::handleDisconnected() {
}

void AndroidMachine::handleCaseRequest(const CastRequest &req) {
    auto androidMainWindow = manager()->getAndroidMainWindow();
    int rStage = req.stage();
    qDebug() << "rStage:" << rStage;
    if (rStage & ANDROID_STAGE_TCPIP) {
        androidMainWindow->setWirelessDbgAddress(QString::fromStdString(m_ip), 5545);
        androidMainWindow->connectTCPAdb();
    } else if (rStage & ANDROID_STAGE_USB_ADB) {
        androidMainWindow->setWirelessDbgAddress(QString::fromStdString(m_ip), 5545);
        androidMainWindow->openTCPAdb();
    }
}

void AndroidMachine::handleTransferResponse(const TransferResponse &resp) {
    uint32_t transferId = resp.transferid();

    if (!resp.accepted()) {
        // TODO:
        m_sendTransfers.erase(transferId);
        return;
    }

    auto iter = m_sendTransfers.find(transferId);
    if (iter == m_sendTransfers.end()) {
        return;
    }

    auto &[_, transfer] = *iter;

    transfer->send(m_ip, resp.port());
}

void AndroidMachine::sendFiles(const QStringList &filePaths) {
    m_currentTransferId++;
    uint32_t transferId = m_currentTransferId;
    m_sendTransfers.emplace(transferId, std::make_unique<SendTransfer>(filePaths));

    Message msg;
    auto *transferRequest = msg.mutable_transferrequest();
    transferRequest->set_transferid(transferId);

    sendMessage(msg);
}
