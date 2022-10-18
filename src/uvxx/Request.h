#ifndef UVXX_REQUEST_H
#define UVXX_REQUEST_H

#include <memory>

#include <uv.h>

#include "utils.h"
#include "UvType.h"

struct sockaddr;

namespace uvxx {

class Addr;

typedef void (*uv_write_cb)(uv_write_t *req, int status);

class Request : public UvType<uv_any_req, uv_req_t>, public std::enable_shared_from_this<Request> {
public:
    bool cancel();

    void onSuccess(const std::function<void()> &cb) { successCb_ = cb; }
    void onError(const std::function<void(const std::string &title, const std::string &msg)> &cb) {
        errorCb_ = cb;
    };

protected:
    using UvType::UvType;

    void stay();
    void stopStay();

    std::function<void()> successCb_;
    std::function<void(const std::string &title, const std::string &msg)> errorCb_;

private:
    std::shared_ptr<void> self_;
};

template <class D, typename T>
class RequestT : public Request {
    template <class, typename>
    friend class RequestT;

public:
    static std::shared_ptr<D> create() { return std::shared_ptr<D>(new D()); }

protected:
    using Request::Request;

    T *get() { return Request::get<T>(); }

    static void callback(T *req, int status) {
        auto pptr = static_cast<RequestT *>(req->data)->shared_from_this();
        auto ptr = std::static_pointer_cast<RequestT>(pptr);
        ptr->stopStay();

        if (status != 0) {
            if (ptr->errorCb_) {
                ptr->errorCb_(uv_err_name(status), uv_strerror(status));
            }
            return;
        }

        if (ptr->successCb_) {
            ptr->successCb_();
        }
    }

    template <typename... Args>
    bool invoke(block_deduction<int (*)(Args..., decltype(&RequestT::callback))> func,
                Args &&...args) {
        int ret = std::forward<decltype(func)>(func)(std::forward<Args>(args)...,
                                                     &RequestT::callback);
        if (ret != 0) {
            if (errorCb_) {
                errorCb_(uv_err_name(ret), uv_strerror(ret));
            }
            return false;
        }

        stay();
        return true;
    }
};

class ShutdownRequest final : public RequestT<ShutdownRequest, uv_shutdown_t> {
public:
    bool shutdown(uv_stream_t *handle);

private:
    using RequestT::RequestT;
};

class ConnectRequest final : public RequestT<ConnectRequest, uv_connect_t> {
public:
    template <typename... Args>
    bool connect(
        block_deduction<int (*)(uv_connect_t *, Args..., decltype(&RequestT::callback))> func,
        Args &&...args) {
        return invoke(func, get(), std::forward<Args>(args)...);
    }

private:
    using RequestT::RequestT;
};

class SendRequest : public RequestT<SendRequest, uv_udp_send_t> {
public:
    bool send(uv_udp_t *handle,
              const uv_buf_t *bufs,
              unsigned int nbufs,
              const struct sockaddr *addr);
};

class WriteRequest : public RequestT<WriteRequest, uv_write_t> {
public:
    bool write(uv_stream_t *handle, const uv_buf_t bufs[], unsigned int nbufs);
    bool write2(uv_stream_t *handle,
                const uv_buf_t bufs[],
                unsigned int nbufs,
                uv_stream_t *send_handle);
};

} // namespace uvxx

#endif // !UVXX_REQUEST_H
