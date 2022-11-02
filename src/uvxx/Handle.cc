#include "Handle.h"

#include "Loop.h"

using namespace uvxx;

Handle::Handle(const std::shared_ptr<Loop> &loop, std::string &&typeName)
    : UvType(std::forward<std::string>(typeName))
    , loop_(loop) {
}

Handle::~Handle() {
    if (!uv_is_closing(get())) {
        if (closedCb_) {
            spdlog::error("closed callback is setted, close manual!");
        }

        uv_close(get(), nullptr);
    }
}

uv_loop_t *Handle::getLoop() {
    return loop_->get();
}

void Handle::close() {
    if (!uv_is_closing(get())) {
        uv_close(get(), CallbackWrapper<&Handle::closeCb>::func);
    }
}

void Handle::closeCb([[maybe_unused]] uv_handle_t *handle) {
    if (closedCb_) {
        closedCb_();
    }
}
