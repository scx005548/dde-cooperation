#ifndef UTILS_MESSAGE_H
#define UTILS_MESSAGE_H

#include <giomm.h>
#include <spdlog/spdlog.h>

#include "protocol/base_message.pb.h"

#define SCAN_KEY "UOS-COOPERATION"

extern std::shared_ptr<spdlog::logger> logger;

namespace Message {

inline void send_message(const Glib::RefPtr<Gio::Socket> &sock,
                         uint32_t type,
                         const google::protobuf::Message &response) noexcept {
    BaseMessage base;
    base.set_type(type);
    base.set_size(response.ByteSizeLong());

    char *buffer = new char[base.ByteSizeLong()];
    base.SerializeToArray(buffer, base.ByteSizeLong());
    try {
        sock->send(buffer, base.ByteSizeLong());
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    delete[] buffer;

    buffer = new char[response.ByteSizeLong()];
    response.SerializeToArray(buffer, response.ByteSizeLong());
    try {
        sock->send(buffer, response.ByteSizeLong());
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    delete[] buffer;
}

inline BaseMessage recv_message_header(const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    static const size_t base_msg_size = 14;
    BaseMessage base;
    char *buffer = new char[base_msg_size];
    try {
        sock->receive(buffer, base_msg_size);
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    base.ParseFromArray(buffer, base_msg_size);
    delete[] buffer;

    return base;
}

template <typename T>
inline T recv_message_body(const Glib::RefPtr<Gio::Socket> &sock,
                           const BaseMessage &base) noexcept {
    char *buffer = new char[base.size()];
    try {
        sock->receive(buffer, base.size());
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    T response;
    response.ParseFromArray(buffer, base.size());
    delete[] buffer;

    return response;
}

template <typename T>
inline T recv_message(const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    static const size_t base_msg_size = 14;
    BaseMessage base;
    char *buffer = new char[base_msg_size];
    try {
        sock->receive(buffer, base_msg_size);
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    base.ParseFromArray(buffer, base_msg_size);
    delete[] buffer;

    buffer = new char[base.size()];
    try {
        sock->receive(buffer, base.size());
    } catch (Gio::Error &e) {
        logger->error("{}", e.what().c_str());
    }
    T response;
    response.ParseFromArray(buffer, base.size());
    delete[] buffer;

    return response;
}

template <typename T>
inline void send_message_to(const Glib::RefPtr<Gio::Socket> &sock,
                            uint32_t type,
                            const T &message,
                            const Glib::RefPtr<Gio::SocketAddress> &addr) noexcept {
    BaseMessage base;
    base.set_type(type);
    base.set_size(message.ByteSizeLong());

    char *buffer = new char[base.ByteSizeLong() + message.ByteSizeLong()];
    base.SerializeToArray(buffer, base.ByteSizeLong());
    message.SerializeToArray(buffer + base.ByteSizeLong(), message.ByteSizeLong());
    sock->send_to(addr, buffer, base.ByteSizeLong() + message.ByteSizeLong());
    delete[] buffer;
}

template <typename T>
inline T recv_message_from(const Glib::RefPtr<Gio::Socket> &sock,
                           Glib::RefPtr<Gio::SocketAddress> &addr) noexcept {
    static const size_t base_msg_size = 14;
    BaseMessage base;
    char *buffer = new char[65535];
    sock->receive_from(addr, buffer, 65535);
    base.ParseFromArray(buffer, base_msg_size);
    T message;
    message.ParseFromArray(buffer + base_msg_size, base.size());
    delete[] buffer;

    return message;
}

} // namespace Message

#endif // !UTILS_MESSAGE_H
