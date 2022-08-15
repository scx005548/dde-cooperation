#include "Signal.h"

using namespace uvxx;

Signal::Signal(const std::shared_ptr<Loop> &loop)
    : HandleT(loop) {
    initialize(&uv_signal_init);
}

bool Signal::start(int signum) {
    return invoke(&uv_signal_start, get(), &CallbackWrapper<&Signal::signalCb>::func, signum);
}

bool Signal::oneshot(int signum) {
    return invoke(&uv_signal_start_oneshot,
                  get(),
                  &CallbackWrapper<&Signal::signalCb>::func,
                  signum);
}

void Signal::signalCb([[maybe_unused]] uv_signal_t *handle, int signum) {
    triggerCb_(signum);
}
