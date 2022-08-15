#ifndef UVXX_STREAM_H
#define UVXX_STREAM_H

#include "Handle.h"

namespace uvxx {

class Addr;

class Stream : public HandleT<uv_stream_t> {
public:
    bool listen();
    bool accept(const std::shared_ptr<Stream> &conn);
    bool shutdown();

    bool startRead();
    bool write(std::vector<char> &&data);

    void onNewConnection(const std::function<void(bool)> &cb) { newConnectionCb_ = cb; }
    void onReceived(const std::function<void(std::unique_ptr<char[]> buffer, ssize_t size)> &cb) {
        receivedCb_ = cb;
    }

protected:
    using HandleT::HandleT;

    void allocCb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    void readCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

private:
    std::function<void(bool)> newConnectionCb_{nullFunc{}};
    std::function<void(std::unique_ptr<char[]> buffer, ssize_t size)> receivedCb_{nullFunc{}};

    void newConnectionCb(uv_stream_t *req, int status);
    void bufferedReadCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
};

template <class D, typename T>
class StreamT : public Stream {
public:
    std::shared_ptr<D> accept() {
        auto conn = std::make_shared<D>(loop_);
        Stream::accept(conn);
        // TODO: error handling

        return conn;
    }

protected:
    using Stream::allocCb;
    using Stream::Stream;

    T *get() { return Handle::get<T>(); }
};

} // namespace uvxx

#endif // !UVXX_STREAM_H
