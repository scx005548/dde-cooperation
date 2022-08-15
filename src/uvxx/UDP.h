#ifndef UVXX_UDP_H
#define UVXX_UDP_H

#include "Stream.h"

namespace uvxx {

class Addr;

class UDP : public StreamT<UDP, uv_udp_t> {
public:
    explicit UDP(const std::shared_ptr<Loop> &loop);

    bool bind(const std::shared_ptr<Addr> &addr);
    bool bind(const std::string &ip, uint16_t port = 0);
    bool listen() = delete;

    bool connect(const std::shared_ptr<Addr> &addr);
    bool connect(const std::string &ip, uint16_t port);

    bool setBroadcast(bool on);

    bool startRecv();
    bool stopRecv();

    bool send(const std::shared_ptr<Addr> &addr, const std::vector<char> &data);

    void onSendFailed(
        const std::function<void(const std::string &title, const std::string &msg)> &cb) {
        sendFailedCb_ = cb;
    };
    void onReceived(const std::function<void(std::shared_ptr<uvxx::Addr> addr,
                                             std::unique_ptr<char[]> data,
                                             size_t size,
                                             bool partial)> &cb) {
        receivedCb_ = cb;
    }

protected:
private:
    std::function<void(const std::string &title, const std::string &msg)> sendFailedCb_{nullFunc{}};
    std::function<void(std::shared_ptr<uvxx::Addr> addr,
                       std::unique_ptr<char[]> data,
                       size_t size,
                       bool partial)>
        receivedCb_{nullFunc{}};

    void recvCb(uv_udp_t *handle,
                ssize_t nread,
                const uv_buf_t *buf,
                const struct sockaddr *sa,
                unsigned flags);
};

} // namespace uvxx

#endif // !UVXX_UDP_H
