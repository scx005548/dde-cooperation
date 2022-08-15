#include "Handle.h"

#include "Loop.h"

using namespace uvxx;

Handle::Handle(const std::shared_ptr<Loop> &loop)
    : UvType()
    , loop_(loop) {
}

Handle::~Handle() {
    close();
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
}
