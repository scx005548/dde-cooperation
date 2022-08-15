#include "InputEmittorWrapper.h"

#include "config.h"
#include "utils/message_helper.h"

#include "uvxx/Pipe.h"
#include "uvxx/Process.h"

#include "protocol/ipc_message.pb.h"

InputEmittorWrapper::InputEmittorWrapper(const std::weak_ptr<Machine> &machine,
                                         const std::shared_ptr<uvxx::Loop> &uvLoop,
                                         InputDeviceType type)
    : m_machine(machine)
    , m_uvLoop(uvLoop)
    , m_pipe(std::make_shared<uvxx::Pipe>(m_uvLoop, true))
    , m_process(std::make_shared<uvxx::Process>(m_uvLoop, INPUT_EMITTOR_PATH))
    , m_type(type) {
    m_process->setStdout(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stdout));
    m_process->setStderr(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stderr));
    int fd = m_process->addStdio(
        static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE),
        std::static_pointer_cast<uvxx::Stream>(m_pipe));
    m_process->args.emplace_back(std::to_string(fd));
    m_process->args.emplace_back(std::to_string(uint8_t(type)));
    m_process->spawn();

    m_pipe->onReceived(uvxx::memFunc(this, &InputEmittorWrapper::onReceived));
    m_pipe->startRead();
}

bool InputEmittorWrapper::emitEvent(unsigned int type, unsigned int code, int value) noexcept {
    InputEmittorParent msg;
    auto *inputEvent = msg.mutable_inputevent();
    inputEvent->set_type(type);
    inputEvent->set_code(code);
    inputEvent->set_value(value);
    return m_pipe->write(MessageHelper::genMessage(msg));
}

void InputEmittorWrapper::onReceived(std::unique_ptr<char[]> buffer, ssize_t size) noexcept {
    spdlog::info("onReceived");
}
