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

const size_t header_size = sizeof(MessageHeader);

namespace MessageHelper {

inline void send_message(const Glib::RefPtr<Gio::Socket> &sock, const Message &response) noexcept {
    MessageHeader header{.size = response.ByteSizeLong()};
    char *buffer = new char[header_size + response.ByteSizeLong()];
    memcpy(buffer, &header, header_size);

    response.SerializeToArray(buffer, response.ByteSizeLong());
    try {
        sock->send(buffer, response.ByteSizeLong());
    } catch (Gio::Error &e) {
        spdlog::error("{}", e.what().c_str());
    }
    delete[] buffer;
}

inline MessageHeader recv_messageheader(const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    MessageHeader header;
    try {
        sock->receive(reinterpret_cast<char *>(&header), header_size);
    } catch (Gio::Error &e) {
        spdlog::error("{}", e.what().c_str());
    }

    return header;
}

inline Message recv_message_body(const Glib::RefPtr<Gio::Socket> &sock, size_t size) noexcept {
    char *buffer = new char[size];
    try {
        sock->receive(buffer, size);
    } catch (Gio::Error &e) {
        spdlog::error("{}", e.what().c_str());
    }
    Message response;
    response.ParseFromArray(buffer, size);
    delete[] buffer;

    return response;
}

inline Message recv_message(const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    MessageHeader header;
    try {
        sock->receive(reinterpret_cast<char *>(&header), header_size);
    } catch (Gio::Error &e) {
        spdlog::error("{}", e.what().c_str());
    }

    char *buffer = new char[header.size];
    try {
        sock->receive(buffer, header.size);
    } catch (Gio::Error &e) {
        spdlog::error("{}", e.what().c_str());
    }
    Message response;
    response.ParseFromArray(buffer, header.size);
    delete[] buffer;

    return response;
}

inline void send_message_to(const Glib::RefPtr<Gio::Socket> &sock,
                            const Message &message,
                            const Glib::RefPtr<Gio::SocketAddress> &addr) noexcept {
    MessageHeader header{.size = message.ByteSizeLong()};

    char *buffer = new char[header_size + message.ByteSizeLong()];
    memcpy(buffer, &header, header_size);
    message.SerializeToArray(buffer + header_size, message.ByteSizeLong());
    sock->send_to(addr, buffer, header_size + message.ByteSizeLong());
    delete[] buffer;
}

inline Message recv_message_from(const Glib::RefPtr<Gio::Socket> &sock,
                                 Glib::RefPtr<Gio::SocketAddress> &addr) noexcept {
    MessageHeader header;
    Message base;
    sock->receive(reinterpret_cast<char *>(&header), header_size);

    char *buffer = new char[header.size];
    sock->receive_from(addr, buffer, header.size);
    Message message;
    message.ParseFromArray(buffer, header.size);
    delete[] buffer;

    return message;
}

} // namespace MessageHelper

#endif // !UTILS_MESSAGE_H
