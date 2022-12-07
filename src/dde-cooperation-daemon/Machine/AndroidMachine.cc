#include "AndroidMachine.h"

#include <QProcess>

#include "config.h"

AndroidMachine::AndroidMachine(Manager *manager,
                               ClipboardBase *clipboard,
                               const std::shared_ptr<uvxx::Loop> &uvLoop,
                               QDBusConnection service,
                               uint32_t id,
                               const std::filesystem::path &dataDir,
                               const std::string &ip,
                               uint16_t port,
                               const DeviceInfo &sp)
    : Machine(manager, clipboard, uvLoop, service, id, dataDir, ip, port, sp)
    , m_process(new QProcess(this)) {
}

void AndroidMachine::handleConnected() {
    m_process->start(DDE_SCRCPY_PATH, QStringList{QString::fromStdString(m_ip)});
}

void AndroidMachine::handleDisconnected() {
    m_process->kill();
}
