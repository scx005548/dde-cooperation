#include "UDP.h"

#include "utils.h"
#include "Addr.h"
#include "Request.h"

using namespace uvxx;

UDP::UDP(const std::shared_ptr<Loop> &loop)
    : StreamT(loop) {
    initialize(&uv_udp_init);
}

bool UDP::bind(const std::shared_ptr<Addr> &addr) {
    return invoke(&uv_udp_bind, get(), const_cast<const sockaddr *>(addr->get()), 0U);
}

bool UDP::bind(const std::string &ip, uint16_t port) {
    // TODO: check ipv4 or ipv6
    return bind(IPv4Addr::create(ip, port));
}

bool UDP::connect(const std::shared_ptr<Addr> &addr) {
    return invoke(&uv_udp_connect, get(), const_cast<const sockaddr *>(addr->get()));
}

bool UDP::connect(const std::string &ip, uint16_t port) {
    // TODO: check ipv4 or ipv6
    return connect(IPv4Addr::create(ip, port));
}

bool UDP::setBroadcast(bool on) {
    return invoke(uv_udp_set_broadcast, get(), static_cast<int>(on));
}

bool UDP::startRecv() {
    return invoke(&uv_udp_recv_start,
                  get(),
                  &CallbackWrapper<&UDP::allocCb>::func,
                  &CallbackWrapper<&UDP::recvCb>::func);
}

bool UDP::stopRecv() {
    return invoke(&uv_udp_recv_stop, get());
}

bool UDP::send(const std::shared_ptr<Addr> &addr, const std::vector<char> &data) {
    auto req = std::make_shared<SendRequest>();
    req->onError(sendFailedCb_);
    uv_buf_t bufs[] = {uv_buf_init(const_cast<char *>(data.data()), data.size())};

    return req->send(get(), bufs, 1, addr->get());
}

void UDP::allocCb([[maybe_unused]] uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init(new char[suggested_size], suggested_size);
}

void UDP::recvCb([[maybe_unused]] uv_udp_t *handle,
                 ssize_t nread,
                 const uv_buf_t *buf,
                 const struct sockaddr *sa,
                 unsigned flags) {
    if (sa != nullptr) {
        if (receivedCb_) {
            receivedCb_(Addr::create(sa),
                        std::unique_ptr<char[]>{buf->base},
                        nread,
                        ((flags & UV_UDP_PARTIAL) != 0));
        }
    } else {
        // empty
    }
}
