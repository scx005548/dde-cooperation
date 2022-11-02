#include "Handle.h"

#include "Loop.h"

using namespace uvxx;

Handle::Handle(const std::shared_ptr<Loop> &loop, std::string &&typeName)
    : UvType(std::forward<std::string>(typeName))
    , loop_(loop) {
}

Handle::~Handle() {
    if (!uv_is_closing(get())) {
        spdlog::error("{} is not closed", getTypeName());
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

bool Handle::isClosing() {
    return uv_is_closing(get());
}

void Handle::closeCb([[maybe_unused]] uv_handle_t *handle) {
    if (closedCb_) {
        closedCb_();
    }
}
