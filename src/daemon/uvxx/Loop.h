#ifndef UVXX_LOOP_H
#define UVXX_LOOP_H

#include <memory>

#include <uv.h>

#include "noncopyable.h"

namespace uvxx {

class Loop : public noncopyable {
    friend class Handle;

public:
    static std::shared_ptr<Loop> defaultLoop();
    Loop();
    ~Loop();

    bool run();
    void stop();

    bool isRunning() const { return isRunning_; }

protected:
    uv_loop_t *get() { return m_loop.get(); }

private:
    std::unique_ptr<uv_loop_t> m_loop;
    bool isRunning_;
};

} // namespace uvxx

#endif // !UVXX_LOOP_H
