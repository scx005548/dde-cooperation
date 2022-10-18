#ifndef UVXX_TCP_H
#define UVXX_TCP_H

#include "Stream.h"

namespace uvxx {

class TCP : public StreamT<TCP, uv_tcp_t> {
public:
    explicit TCP(const std::shared_ptr<Loop> &loop);

    bool tcpNoDelay(bool enable = true);
    bool keepalive(bool enable, unsigned int delay);

    bool bind(const std::shared_ptr<Addr> &addr);
    bool bind(const std::string &ip, uint16_t port = 0);

    bool connect(const std::shared_ptr<Addr> &addr);
    bool connect(const std::string &ip, uint16_t port);

    std::shared_ptr<Addr> localAddress();
    std::shared_ptr<Addr> remoteAddress();

    void onConnected(const std::function<void()> &cb) { connectedCb_ = cb; }
    void onConnectFailed(
        const std::function<void(const std::string &title, const std::string &msg)> &cb) {
        connectFailedCb_ = cb;
    }

private:
    std::function<void()> connectedCb_;
    std::function<void(const std::string &title, const std::string &msg)> connectFailedCb_;
};

} // namespace uvxx

#endif // !UVXX_TCP_H
