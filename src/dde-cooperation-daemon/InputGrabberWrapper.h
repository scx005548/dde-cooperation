#ifndef DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H
#define DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H

#include <filesystem>

namespace uvxx {
class Loop;
class Pipe;
class Process;
}

class Manager;
class Machine;

class InputGrabberWrapper {
public:
    explicit InputGrabberWrapper(Manager *manager,
                                 const std::shared_ptr<uvxx::Loop> &uvLoop,
                                 const std::filesystem::path &path);
    void setMachine(const std::weak_ptr<Machine> &machine);
    void start();
    void stop();

private:
    Manager *m_manager;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Pipe> m_pipe;
    std::shared_ptr<uvxx::Process> m_process;

    uint8_t m_type;
    std::weak_ptr<Machine> m_machine;

    void onReceived(std::unique_ptr<char[]> buffer, ssize_t size) noexcept;
};

#endif // !DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H
