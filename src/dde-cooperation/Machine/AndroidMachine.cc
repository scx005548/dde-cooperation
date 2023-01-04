#include "AndroidMachine.h"

#include <filesystem>
#include <algorithm>

#include "Manager.h"
#include "AndroidMachineDBusAdaptor.h"
#include "Android/AndroidMainWindow.h"
#include "SendTransfer.h"

AndroidMachine::AndroidMachine(Manager *manager,
                               ClipboardBase *clipboard,
                               QDBusConnection service,
                               uint32_t id,
                               const std::filesystem::path &dataDir,
                               const std::string &ip,
                               uint16_t port,
                               const DeviceInfo &sp)
    : Machine(manager, clipboard, service, id, dataDir, ip, port, sp)
    , m_dbusAdaptorAndroid(new AndroidMachineDBusAdaptor(this, m_bus, m_dbusPath))
    , m_mainWindow(nullptr)
    , m_currentTransferId(0) {
}

void AndroidMachine::startCast() {
    Message msg;
    auto *reverseCastRequest = msg.mutable_reversecastrequest();
    reverseCastRequest->set_uuid(uuid());
    sendMessage(msg);
}

void AndroidMachine::handleConnected() {
}

void AndroidMachine::handleDisconnected() {
    if (m_mainWindow) {
        m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
    }
}

void AndroidMachine::handleCastRequest(const CastRequest &req) {
    if (!m_mainWindow) {
        m_mainWindow = manager()->getAndroidMainWindow();
        m_mainWindow->setParent(this);
    }
    int rStage = req.stage();
    qDebug() << "rStage:" << rStage;

    m_mainWindow->disconnect();
    QObject::connect(m_mainWindow, &AndroidMainWindow::tcpAdbConnected, this, [this]() {
        qDebug() << "tcpAdbConnected";

        Message msg;
        auto *castResponse = msg.mutable_castresponse();
        castResponse->set_well(true);
        sendMessage(msg);
    });
    QObject::connect(m_mainWindow, &AndroidMainWindow::destroyed, this, [this]() {
        m_mainWindow = nullptr;
    });

    if (rStage & ANDROID_STAGE_TCPIP) {
        m_mainWindow->setWirelessDbgAddress(QString::fromStdString(m_ip), 5545);
        m_mainWindow->ensureTCPAdbConnected();
    } else if (rStage & ANDROID_STAGE_USB_ADB) {
        m_mainWindow->setWirelessDbgAddress(QString::fromStdString(m_ip), 5545);
        m_mainWindow->openTCPAdb();
    }
}

void AndroidMachine::handleTransferRequest(const TransferRequest &req) {
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

    auto [_, transfer] = *iter;

    transfer->send(m_ip, resp.port());
}

void AndroidMachine::sendFiles(const QStringList &filePaths) {
    m_currentTransferId++;
    uint32_t transferId = m_currentTransferId;
    auto *transfer = new SendTransfer(filePaths, this);
    m_sendTransfers.emplace(transferId, transfer);

    QObject::connect(transfer, &SendTransfer::destroyed, this, [this, transferId]() {
        m_sendTransfers.erase(transferId);
    });

    Message msg;
    auto *transferRequest = msg.mutable_transferrequest();
    transferRequest->set_transferid(transferId);

    sendMessage(msg);
}
