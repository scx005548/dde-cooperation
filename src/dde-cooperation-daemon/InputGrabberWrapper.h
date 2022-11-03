#ifndef DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H
#define DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H

#include <filesystem>

namespace uvxx {
class Loop;
class Pipe;
class Process;
class Buffer;
} // namespace uvxx

class Manager;
class Machine;

class InputGrabberWrapper {
public:
    explicit InputGrabberWrapper(Manager *manager,
                                 const std::shared_ptr<uvxx::Loop> &uvLoop,
                                 const std::filesystem::path &path);
    ~InputGrabberWrapper();
    void setMachine(const std::weak_ptr<Machine> &machine);
    void start();
    void stop();

private:
    Manager *m_manager;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Pipe> m_pipe;
    std::shared_ptr<uvxx::Process> m_process;

    const std::filesystem::path m_path;

    uint8_t m_type;
    std::weak_ptr<Machine> m_machine;

    void onReceived(uvxx::Buffer &buff) noexcept;
    void onClosed();
};

#endif // !DDE_COOPERATION_DAEMON_INPUTGRABBERWRAPPER_H
