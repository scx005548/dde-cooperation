#include "TCP.h"

#include "utils.h"
#include "Request.h"
#include "Addr.h"

using namespace uvxx;

TCP::TCP(const std::shared_ptr<Loop> &loop)
    : StreamT(loop) {
    initialize(&uv_tcp_init);
}

bool TCP::bind(const std::shared_ptr<Addr> &addr) {
    return invoke(uv_tcp_bind, get(), const_cast<const sockaddr *>(addr->get()), 0u);
}

bool TCP::bind(const std::string &ip, uint16_t port) {
    // TODO: check ipv4 or ipv6
    return bind(IPv4Addr::create(ip, port));
}

bool TCP::connect(const std::shared_ptr<Addr> &addr) {
    spdlog::info("connect to: {}", addr->toString());
    auto req = ConnectRequest::create();
    req->onSuccess(connectedCb_);
    req->onError(connectFailedCb_);
    return req->connect(&uv_tcp_connect, get(), const_cast<const sockaddr *>(addr->get()));
}

bool TCP::connect(const std::string &ip, uint16_t port) {
    // TODO: check ipv4 or ipv6
    return connect(IPv4Addr::create(ip, port));
}

std::shared_ptr<Addr> TCP::localAddress() {
    auto addr = Addr::create();
    int len = sizeof(addr);
    int ret = uv_tcp_getsockname(get(), addr->get(), &len);
    if (ret < 0) {
        // TODO: error handling
    }

    return addr;
}

std::shared_ptr<Addr> TCP::remoteAddress() {
    auto addr = Addr::create();
    int len = sizeof(addr);
    int ret = uv_tcp_getpeername(get(), addr->get(), &len);
    if (ret < 0) {
        // TODO: error handling
    }

    return addr;
}
