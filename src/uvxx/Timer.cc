#include "Timer.h"

using namespace uvxx;

Timer::Timer(const std::shared_ptr<Loop> &loop, const std::function<void()> &cb)
    : HandleT(loop)
    , cb_(cb) {
    initialize(&uv_timer_init);
}

bool Timer::start(uint64_t milliseconds) {
    isOneShot_ = false;
    timeout_ = milliseconds;
    return startAux();
}

bool Timer::oneshot(uint64_t milliseconds) {
    isOneShot_ = true;
    timeout_ = milliseconds;
    return startAux();
}

bool Timer::stop() {
    return invoke(&uv_timer_stop, get());
}

bool Timer::reset() {
    invoke(&uv_timer_stop, get());
    return startAux();
}

bool Timer::startAux() {
    return invoke(&uv_timer_start,
                  get(),
                  &CallbackWrapper<&Timer::timerCb>::func,
                  timeout_,
                  isOneShot_ ? 0 : timeout_);
}

void Timer::timerCb([[maybe_unused]] uv_timer_t *handle) {
    if (cb_) {
        cb_();
    }
}
