#ifndef UVXX_TIMER_H
#define UVXX_TIMER_H

#include "Handle.h"

namespace uvxx {

class Timer : public HandleT<uv_timer_t> {
public:
    explicit Timer(const std::shared_ptr<Loop> &loop, const std::function<void()> &cb);

    bool start(uint64_t milliseconds);
    bool oneshot(uint64_t milliseconds);
    bool stop();

    bool reset();

private:
    bool isOneShot_;
    uint64_t timeout_;
    std::function<void()> cb_;

    bool startAux();
    void timerCb(uv_timer_t *handle);
};

} // namespace uvxx

#endif // !UVXX_TIMER_H
