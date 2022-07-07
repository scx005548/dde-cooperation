#include "Async.h"

#include "utils.h"

using namespace uvxx;

Async::Async(const std::shared_ptr<Loop> &loop)
    : HandleT(loop) {
    initialize(&uv_async_init, &CallbackWrapper<&Async::asyncCb>::func);
}

bool Async::wake(const std::function<void()> &cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cb_queue_.push(cb);
    }
    return invoke(&uv_async_send, get());
}

void Async::asyncCb([[maybe_unused]] uv_async_t *handle) {
    std::function<void()> cb;
    for (;;) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (cb_queue_.empty()) {
                return;
            }

            cb = cb_queue_.front();
            cb_queue_.pop();
        }

        cb();
    }
}
