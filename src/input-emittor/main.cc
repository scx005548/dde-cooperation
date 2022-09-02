#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "InputEmittor.h"

#include "uvxx/Loop.h"
#include "uvxx/Pipe.h"
#include "uvxx/Signal.h"

#include "utils/message_helper.h"
#include "protocol/ipc_message.pb.h"

int main(int argc, const char *argv[]) {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] input-emittor [thread %t]: %v");

    if (argc < 3) {
        spdlog::critical("3 args");
        return 1;
    }

    int fd = atoi(argv[1]);
    InputDeviceType type = static_cast<InputDeviceType>(atoi(argv[2]));
    auto loop = uvxx::Loop::defaultLoop();

    InputEmittor emittor(loop, type);

    auto pipe = std::make_shared<uvxx::Pipe>(loop, true);
    pipe->open(fd);
    pipe->onReceived([&emittor](uvxx::Buffer &buff) {
        while (buff.size() >= header_size) {
            auto res = MessageHelper::parseMessage<InputEmittorParent>(buff);
            if (!res.has_value()) {
                return;
            }

            InputEmittorParent &base = res.value();

            switch (base.payload_case()) {
            case InputEmittorParent::PayloadCase::kInputEvent: {
                auto &inputEvent = base.inputevent();
                emittor.emitEvent(inputEvent.type(), inputEvent.code(), inputEvent.value());
            } break;
            case InputEmittorParent::PayloadCase::PAYLOAD_NOT_SET: {
            } break;
            }
        }
    });
    pipe->startRead();
    pipe->onClosed([]() { exit(1); });

    auto exitCb = [loop]([[maybe_unused]] int signum) { loop->stop(); };
    auto signalInt = std::make_shared<uvxx::Signal>(loop);
    signalInt->onTrigger(exitCb);
    signalInt->start(SIGINT);
    auto signalQuit = std::make_shared<uvxx::Signal>(loop);
    signalQuit->onTrigger(exitCb);
    signalQuit->start(SIGQUIT);
    auto signalTerm = std::make_shared<uvxx::Signal>(loop);
    signalTerm->onTrigger(exitCb);
    signalTerm->start(SIGTERM);
    auto signalHup = std::make_shared<uvxx::Signal>(loop);
    signalHup->onTrigger(exitCb);
    signalHup->start(SIGHUP);

    loop->run();
}
