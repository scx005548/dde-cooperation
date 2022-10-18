#include "Poll.h"

using namespace uvxx;

Poll::Poll(const std::shared_ptr<Loop> &loop, int fd)
    : HandleT(loop) {
    initialize(&uv_poll_init, fd);
}

bool Poll::start(int events) {
    return invoke(&uv_poll_start, get(), events, &CallbackWrapper<&Poll::pollCb>::func);
}

bool Poll::stop() {
    return invoke(&uv_poll_stop, get());
}

void Poll::pollCb([[maybe_unused]] uv_poll_t *handle, int status, int events) {
    if (status != 0) {
        if (errorCb_) {
            errorCb_(uv_err_name(status), uv_strerror(status));
        }
        return;
    }

    if (eventCb_) {
        eventCb_(events);
    }
}
