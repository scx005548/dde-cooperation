#ifndef UVXX_ASYNC_H
#define UVXX_ASYNC_H

#include "Handle.h"

#include <queue>

#include <uv.h>

namespace uvxx {

class Async : public HandleT<uv_async_t> {
public:
    explicit Async(const std::shared_ptr<Loop> &loop);

    bool wake(const std::function<void()> &cb);

private:
    std::mutex mutex_;
    std::queue<std::function<void()>> cb_queue_;

    void asyncCb(uv_async_t *handle);
};

} // namespace uvxx

#endif // !UVXX_ASYNC_H
