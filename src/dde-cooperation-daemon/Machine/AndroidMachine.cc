#include "AndroidMachine.h"

#include <QProcess>

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
