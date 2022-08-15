#include "Loop.h"

#include <uv.h>

using namespace uvxx;

std::shared_ptr<Loop> Loop::defaultLoop() {
    static std::weak_ptr<Loop> loop;
    if (auto l = loop.lock()) {
        return l;
    }

    auto l = std::make_shared<Loop>();
    loop = l;
    return l;
}

Loop::Loop()
    : m_loop(std::make_unique<uv_loop_t>())
    , isRunning_(false) {
    uv_loop_init(m_loop.get());
}

Loop::~Loop() {
    uv_loop_close(m_loop.get());
}

bool Loop::run() {
    isRunning_ = true;
    int ret = uv_run(m_loop.get(), UV_RUN_DEFAULT);
    isRunning_ = false;
    return ret == 0;
}

void Loop::stop() {
    uv_stop(m_loop.get());
    isRunning_ = false;
}
