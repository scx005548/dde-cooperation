#ifndef DDE_COOPERATION_DAEMON_CONFIRMDIALOGWRAPPER_H
#define DDE_COOPERATION_DAEMON_CONFIRMDIALOGWRAPPER_H

#include <memory>
#include <functional>

namespace uvxx {
class Loop;
class Pipe;
class Process;
class Buffer;
} // namespace uvxx

class Manager;
class Machine;

#define ACCEPT 0x01
#define REJECT 0x00

class ConfirmDialogWrapper {
public:
    explicit ConfirmDialogWrapper(const std::string &machineIp,
                                  const std::string &machineName,
                                  const std::shared_ptr<uvxx::Loop> &uvLoop,
                                  const std::function<void(uvxx::Buffer &buff)> &cb);
    ~ConfirmDialogWrapper();

private:
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Pipe> m_pipe;
    std::shared_ptr<uvxx::Process> m_process;
};

#endif // DDE_COOPERATION_DAEMON_CONFIRMDIALOGWRAPPER_H