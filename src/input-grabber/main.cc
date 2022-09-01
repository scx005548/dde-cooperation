#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "InputGrabber.h"
#include "uvxx/Loop.h"
#include "uvxx/Pipe.h"
#include "uvxx/Signal.h"

#include "utils/message_helper.h"
#include "protocol/ipc_message.pb.h"

int main(int argc, const char *argv[]) {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] input-grabber [thread %t]: %v");

    if (argc < 3) {
        spdlog::critical("3 args");
        return 1;
    }

    int fd = atoi(argv[1]);
    std::string path = argv[2];
    auto loop = uvxx::Loop::defaultLoop();

    InputGrabber grabber(loop, path);
    if (grabber.shouldIgnore()) {
        return 2;
    }

    auto pipe = std::make_shared<uvxx::Pipe>(loop, true);
    pipe->open(fd);
    pipe->onReceived([&grabber](uvxx::Buffer &buff) {
        while (buff.size() >= header_size) {
            auto res = MessageHelper::parseMessage<InputGrabberParent>(buff);
            if (!res.has_value()) {
                return;
            }

            InputGrabberParent &base = res.value();

            switch (base.payload_case()) {
            case InputGrabberParent::PayloadCase::kStart: {
                grabber.start();
            } break;
            case InputGrabberParent::PayloadCase::kStop: {
                grabber.stop();
            } break;
            case InputGrabberParent::PayloadCase::PAYLOAD_NOT_SET: {
            } break;
            }
        }
    });
    pipe->startRead();
    pipe->onClosed([]() { exit(1); });

    grabber.onEvent([pipe](unsigned int type, unsigned int code, int value) {
        InputGrabberChild msg;
        auto *inputEvent = msg.mutable_inputevent();
        inputEvent->set_type(type);
        inputEvent->set_code(code);
        inputEvent->set_value(value);
        pipe->write(MessageHelper::genMessage(msg));
    });

    InputGrabberChild msg;
    msg.set_devicetype(static_cast<uint8_t>(grabber.type()));
    pipe->write(MessageHelper::genMessage(msg));

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
