#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "InputEmittor.h"

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
    InputDeviceType type = static_cast<InputDeviceType>(atoi(argv[2]));
    auto loop = uvxx::Loop::defaultLoop();

    InputEmittor emittor(loop, type);

    auto pipe = std::make_shared<uvxx::Pipe>(loop, false);
    pipe->open(fd);
    pipe->startRead();
    pipe->onReceived([&emittor](std::unique_ptr<char[]> data, [[maybe_unused]] ssize_t size) {
        auto buff = data.get();
        auto header = MessageHelper::parseMessageHeader(buff);
        buff += header_size;
        size -= header_size;

        auto base = MessageHelper::parseMessageBody<InputEmittorParent>(buff, header.size);
        switch (base.payload_case()) {
        case InputEmittorParent::PayloadCase::kInputEvent: {
            auto &inputEvent = base.inputevent();
            emittor.emitEvent(inputEvent.type(), inputEvent.code(), inputEvent.value());
        } break;
        case InputEmittorParent::PayloadCase::PAYLOAD_NOT_SET: {
        } break;
        }
    });

    loop->run();
}
