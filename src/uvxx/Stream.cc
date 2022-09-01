#include "Stream.h"

#include "Request.h"

using namespace uvxx;

bool Stream::listen() {
    return invoke(&uv_listen, get(), SOMAXCONN, &CallbackWrapper<&Stream::newConnectionCb>::func);
}

bool Stream::accept(const std::shared_ptr<Stream> &conn) {
    // TODO: connection id
    return invoke(&uv_accept, get(), conn->get());
}

bool Stream::shutdown() {
    auto req = ShutdownRequest::create();
    return req->shutdown(get());
}

bool Stream::startRead() {
    int ret = uv_read_start(get(),
                            &CallbackWrapper<&Stream::allocCb>::func,
                            &CallbackWrapper<&Stream::readCb>::func);
    return ret == 0;
}

bool Stream::write(std::vector<char> &&data) {
    auto req = WriteRequest::create();
    uv_buf_t bufs[] = {uv_buf_init(data.data(), data.size())};

    return req->write(get(), bufs, 1);
}

void Stream::newConnectionCb([[maybe_unused]] uv_stream_t *req, int status) {
    newConnectionCb_(status == 0);
}

void Stream::allocCb([[maybe_unused]] uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buff_.expand(suggested_size);

    buf->base = buff_.writable();
    buf->len = buff_.writableSize();
}

void Stream::readCb([[maybe_unused]] uv_stream_t *stream,
                    ssize_t nread,
                    [[maybe_unused]] const uv_buf_t *buf) {
    if (nread == UV_EOF) {
        close();
        return;
    }

    buff_.writed(nread);
    receivedCb_(buff_);
}
