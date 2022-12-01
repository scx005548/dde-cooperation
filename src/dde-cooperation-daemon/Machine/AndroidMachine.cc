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
    : Machine(manager, clipboard, service, id, dataDir, ip, port, sp) {
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
