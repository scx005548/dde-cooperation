#ifndef UVXX_SIGNAL_H
#define UVXX_SIGNAL_H

#include <functional>
#include <memory>

#include "Handle.h"

namespace uvxx {

class Signal : public HandleT<uv_signal_t> {
public:
    explicit Signal(const std::shared_ptr<Loop> &loop);

    void onTrigger(const std::function<void(int signum)> &cb) { triggerCb_ = cb; };

    bool start(int signum);
    bool oneshot(int signum);

private:
    std::function<void(int signum)> triggerCb_;

    void signalCb(uv_signal_t *handle, int signum);
};

} // namespace uvxx

#endif // !UVXX_SIGNAL_H
