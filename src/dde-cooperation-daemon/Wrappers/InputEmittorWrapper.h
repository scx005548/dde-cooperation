#ifndef WRAPPERS_INPUTEMITTORWRAPPER_H
#define WRAPPERS_INPUTEMITTORWRAPPER_H

#include <filesystem>

#include "common.h"

namespace uvxx {
class Loop;
class Pipe;
class Process;
class Buffer;
} // namespace uvxx

class Manager;
class Machine;

class InputEmittorWrapper {
public:
    explicit InputEmittorWrapper(const std::weak_ptr<Machine> &machine,
                                 const std::shared_ptr<uvxx::Loop> &uvLoop,
                                 InputDeviceType type);
    ~InputEmittorWrapper();
    void setMachine(const std::weak_ptr<Machine> &machine);
    bool emitEvent(unsigned int type, unsigned int code, int value) noexcept;

private:
    std::weak_ptr<Machine> m_machine;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Pipe> m_pipe;
    std::shared_ptr<uvxx::Process> m_process;

    InputDeviceType m_type;

    void onReceived(uvxx::Buffer &buff) noexcept;
    void onProcessClosed();
    void onPipeClosed();
};

#endif // !WRAPPERS_INPUTEMITTORWRAPPER_H
