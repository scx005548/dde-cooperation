#ifndef UTILS_MESSAGE_H
#define UTILS_MESSAGE_H

#include <memory>
#include <optional>

#include <arpa/inet.h>

#include <google/protobuf/message.h>

#include "uvxx/Buffer.h"

#define SCAN_KEY "UOS-COOPERATION"

#if __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) (((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

#pragma pack(push, 1)
struct MessageHeader {
    uint64_t size_;

    MessageHeader(uint64_t size = 0)
        : size_(htonll(size)) {}

    uint64_t size() const { return ntohll(size_); }
};
#pragma pack(pop)

const ssize_t header_size = sizeof(MessageHeader);

namespace MessageHelper {

inline std::vector<char> genMessage(const google::protobuf::Message &msg) {
    MessageHeader header(msg.ByteSizeLong());

    std::vector<char> buff(header_size + msg.ByteSizeLong());
    memcpy(buff.data(), &header, header_size);
    msg.SerializeToArray(buff.data() + header_size, msg.ByteSizeLong());

    return buff;
}

inline const MessageHeader &parseMessageHeader(const char *buffer) noexcept {
    return *reinterpret_cast<const MessageHeader *>(buffer);
}

template <typename T>
inline T parseMessageBody(const char *buffer, size_t size) noexcept {
    T msg;
    msg.ParseFromArray(buffer, size);

    return msg;
}

template <typename T>
inline std::optional<T> parseMessage(uvxx::Buffer &buff) {
    auto header = buff.peak<MessageHeader>();
    if (buff.size() < header_size + header.size()) {
        return std::nullopt;
    }

    T msg;
    msg.ParseFromArray(buff.data() + header_size, header.size());
    buff.retrieve(header_size + header.size());

    return msg;
}

} // namespace MessageHelper

#endif // !UTILS_MESSAGE_H
