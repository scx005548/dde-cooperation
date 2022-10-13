#include "ConfirmDialogWrapper.h"

#include "config.h"

#include "uvxx/Pipe.h"
#include "uvxx/Process.h"

ConfirmDialogWrapper::ConfirmDialogWrapper(const std::string &machineIp, const std::string &machineName,
                                           const std::shared_ptr<uvxx::Loop> &uvLoop,
                                           const std::function<void(uvxx::Buffer &)> &cb)
    : m_uvLoop(uvLoop)
    , m_pipe(std::make_shared<uvxx::Pipe>(m_uvLoop, true))
    , m_process(std::make_shared<uvxx::Process>(m_uvLoop, CONFIRM_DIALOG_PATH)) {
    m_process->setStdout(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stdout));
    m_process->setStderr(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stderr));
    int fd = m_process->addStdio(
        static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE),
        std::static_pointer_cast<uvxx::Stream>(m_pipe));

    m_process->args.emplace_back(machineIp);
    m_process->args.emplace_back(machineName);
    m_process->args.emplace_back(std::to_string(fd));
    m_process->spawn();

    m_pipe->onReceived(cb);
    m_pipe->startRead();
}
