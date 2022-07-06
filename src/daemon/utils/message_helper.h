#ifndef UTILS_MESSAGE_H
#define UTILS_MESSAGE_H

#include <memory>

#include <giomm.h>
#include <spdlog/spdlog.h>

#include "protocol/message.pb.h"

#include "uvxx/UDP.h"
#include "uvxx/TCP.h"

#define SCAN_KEY "UOS-COOPERATION"

#pragma pack(push, 1)
struct MessageHeader {
    uint64_t size;
};
#pragma pack(pop)

const ssize_t header_size = sizeof(MessageHeader);

namespace MessageHelper {

inline std::vector<char> genMessage(const Message &msg) {
#if __cplusplus < 202002L
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++20-designator"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
    MessageHeader header{.size = msg.ByteSizeLong()};
#if __cplusplus < 202002L
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    std::vector<char> buff(header_size + msg.ByteSizeLong());
    memcpy(buff.data(), &header, header_size);
    msg.SerializeToArray(buff.data() + header_size, msg.ByteSizeLong());

    return buff;
}

inline MessageHeader parseMessageHeader(const char *buffer) noexcept {
    MessageHeader header;
    memcpy(&header, buffer, header_size);
    return header;
}

inline Message parseMessageBody(const char *buffer, size_t size) noexcept {
    Message msg;
    msg.ParseFromArray(buffer, size);

    return msg;
}

// template <typename T>
// inline T parseMessage(const char *buffer, size_t size) noexcept {
//     assert(size >= base_msg_size);

//     BaseMessage base;
//     base.ParseFromArray(buffer, base_msg_size);

//     assert(size >= base_msg_size + base.size());

//     T response;
//     response.ParseFromArray(buffer + base_msg_size, size - base_msg_size);

//     return response;
// }

} // namespace MessageHelper

#endif // !UTILS_MESSAGE_H
