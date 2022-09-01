#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "InputGrabber.h"
#include "uvxx/Loop.h"
#include "uvxx/Pipe.h"

#include "utils/message_helper.h"
#include "protocol/ipc_message.pb.h"

int main(int argc, const char *argv[]) {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%^%l%$] [thread %t]: %v");

    if (argc < 3) {
        spdlog::critical("3 args");
        return 1;
    }

    int fd = atoi(argv[1]);
    std::string type = argv[2];
    auto loop = uvxx::Loop::defaultLoop();

    InputGrabber grabber(loop, type);
    if (grabber.shouldIgnore()) {
        return 0;
    }

    auto pipe = std::make_shared<uvxx::Pipe>(loop, false);
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

    grabber.onEvent([pipe](unsigned int type, unsigned int code, int value) {
        InputGrabberChild msg;
        auto *inputEvent = msg.mutable_inputevent();
        inputEvent->set_type(type);
        inputEvent->set_code(code);
        inputEvent->set_value(value);
        pipe->write(MessageHelper::genMessage(msg));
    });

    loop->run();
}
