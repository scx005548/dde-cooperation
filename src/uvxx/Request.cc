#include "Request.h"

#include <uv.h>

using namespace uvxx;

bool Request::cancel() {
    int ret = uv_cancel(get());
    return ret == 0;
}

bool ShutdownRequest::shutdown(uv_stream_t *handle) {
    return invoke(&uv_shutdown, get(), std::forward<uv_stream_t *>(handle));
}

bool SendRequest::send(uv_udp_t *handle,
                       const uv_buf_t *bufs,
                       unsigned int nbufs,
                       const struct sockaddr *addr) {
    return invoke(&uv_udp_send,
                  get(),
                  std::forward<uv_udp_t *>(handle),
                  std::forward<const uv_buf_t *>(bufs),
                  std::forward<unsigned int>(nbufs),
                  std::forward<const sockaddr *>(addr));
}

bool WriteRequest::write(uv_stream_t *handle, const uv_buf_t bufs[], unsigned int nbufs) {
    return invoke(&uv_write,
                  get(),
                  std::forward<uv_stream_t *>(handle),
                  std::forward<const uv_buf_t *>(bufs),
                  std::forward<unsigned int>(nbufs));
}

bool WriteRequest::write2(uv_stream_t *handle,
                          const uv_buf_t bufs[],
                          unsigned int nbufs,
                          uv_stream_t *send_handle) {
    return invoke(&uv_write2,
                  get(),
                  std::forward<uv_stream_t *>(handle),
                  std::forward<const uv_buf_t *>(bufs),
                  std::forward<unsigned int>(nbufs),
                  std::forward<uv_stream_t *>(send_handle));
}
