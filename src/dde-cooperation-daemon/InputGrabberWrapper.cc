#include "InputGrabberWrapper.h"

#include "uvxx/Process.h"
#include "uvxx/Pipe.h"

#include "config.h"
#include "utils/message_helper.h"
#include "Manager.h"
#include "Machine.h"
#include "protocol/ipc_message.pb.h"

InputGrabberWrapper::InputGrabberWrapper(Manager *manager,
                                         const std::shared_ptr<uvxx::Loop> &uvLoop,
                                         const std::filesystem::path &path)
    : m_manager(manager)
    , m_uvLoop(uvLoop)
    , m_pipe(std::make_shared<uvxx::Pipe>(m_uvLoop, true))
    , m_process(std::make_shared<uvxx::Process>(m_uvLoop, INPUT_GRABBER_PATH))
    , m_path(path) {
    m_process->setStdout(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stdout));
    m_process->setStderr(static_cast<uv_stdio_flags>(UV_INHERIT_FD), fileno(stderr));
    int fd = m_process->addStdio(
        static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE),
        std::static_pointer_cast<uvxx::Stream>(m_pipe));
    m_process->args.emplace_back(std::to_string(fd));
    m_process->args.emplace_back(path.string());
    m_process->spawn();
    m_process->onExit([this](int64_t, int) { m_process->close(); });

    m_process->onClosed(uvxx::memFunc(this, &InputGrabberWrapper::onClosed));

    m_pipe->onReceived(uvxx::memFunc(this, &InputGrabberWrapper::onReceived));
    m_pipe->onClosed(uvxx::memFunc(this, &InputGrabberWrapper::onClosed));
    m_pipe->startRead();
}

InputGrabberWrapper::~InputGrabberWrapper() {
    m_pipe->onClosed(nullptr);
    m_process->onExit(nullptr);
    m_process->onClosed(nullptr);

    m_pipe->close();
    m_process->kill(SIGINT);
    m_process->close();
}

void InputGrabberWrapper::setMachine(const std::weak_ptr<Machine> &machine) {
    m_machine = machine;
}

void InputGrabberWrapper::start() {
    InputGrabberParent msg;
    msg.mutable_start();
    m_pipe->write(MessageHelper::genMessage(msg));
}

void InputGrabberWrapper::stop() {
    InputGrabberParent msg;
    msg.mutable_stop();
    m_pipe->write(MessageHelper::genMessage(msg));
}

void InputGrabberWrapper::onReceived(uvxx::Buffer &buff) noexcept {
    spdlog::info("onReceived");

    while (buff.size() >= header_size) {
        auto res = MessageHelper::parseMessage<InputGrabberChild>(buff);
        if (!res.has_value()) {
            return;
        }

        InputGrabberChild &base = res.value();

        switch (base.payload_case()) {
        case InputGrabberChild::PayloadCase::kDeviceType: {
            m_type = base.devicetype();
            break;
        }
        case InputGrabberChild::PayloadCase::kInputEvent: {
            auto machine = m_machine.lock();
            if (machine) {
                auto inputEvent = base.inputevent();
                machine->onInputGrabberEvent(m_type,
                                             inputEvent.type(),
                                             inputEvent.code(),
                                             inputEvent.value());
            }
            break;
        }
        case InputGrabberChild::PAYLOAD_NOT_SET: {
            break;
        }
        }
    }
}

void InputGrabberWrapper::onClosed() {
    m_manager->removeInputGrabber(m_path);
}
