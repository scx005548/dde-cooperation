#include "AndroidMachine.h"

#include "config.h"
#include "uvxx/Process.h"

AndroidMachine::AndroidMachine(Manager *manager,
                               ClipboardBase *clipboard,
                               const std::shared_ptr<uvxx::Loop> &uvLoop,
                               QDBusConnection service,
                               uint32_t id,
                               const std::filesystem::path &dataDir,
                               const std::string &ip,
                               uint16_t port,
                               const DeviceInfo &sp)
    : Machine(manager, clipboard, uvLoop, service, id, dataDir, ip, port, sp) {
}

void AndroidMachine::handleConnected() {
    m_process = std::make_shared<uvxx::Process>(m_uvLoop, DDE_SCRCPY_PATH);
    m_process->setStdout(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stdout));
    m_process->setStderr(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stderr));
    m_process->args.emplace_back(m_ip);

    m_process->spawn();
}

void AndroidMachine::handleDisconnected() {
    // if (m_process) {
    //     m_process->kill(SIGKILL);
    //     m_process->close();
    //     m_process.reset();
    // }
}
