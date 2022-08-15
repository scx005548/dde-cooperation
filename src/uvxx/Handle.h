#ifndef UVXX_UVOBJECT_H
#define UVXX_UVOBJECT_H

#include <uv.h>

#include "utils.h"
#include "UvType.h"

namespace uvxx {

class Loop;

class Handle : public UvType<uv_any_handle, uv_handle_t> {
public:
    void close();

    void onError(const std::function<void(const std::string &title, const std::string &msg)> &cb) {
        errorCb_ = cb;
    };
    void onClosed(const std::function<void()> &cb) { closedCb_ = cb; }

protected:
    explicit Handle(const std::shared_ptr<Loop> &loop);
    ~Handle();

    std::function<void(const std::string &title, const std::string &msg)> errorCb_{nullFunc{}};

    template <typename T, typename... Args>
    bool initialize(int (*func)(uv_loop_t *, T *, Args...), Args... args) {
        int ret = func(getLoop(), get<T>(), std::forward<Args>(args)...);
        if (ret != 0) {
            spdlog::error("init failed: {}, {}", uv_err_name(ret), uv_strerror(ret));
            return false;
        }

        return true;
    }

    template <typename Callable, typename... Args>
    bool invoke(Callable &&func, Args &&...args) {
        int ret = std::invoke(std::forward<Callable>(func), std::forward<Args>(args)...);
        if (ret != 0) {
            // errorCb_(uv_err_name(ret), uv_strerror(ret));
            spdlog::error("{}, {}, {}",
                          typeid(decltype(func)).name(),
                          uv_err_name(ret),
                          uv_strerror(ret));
            return false;
        }

        // stay();
        return true;
    }
    std::shared_ptr<Loop> loop_;

private:
    std::function<void()> closedCb_{nullFunc{}};

    uv_loop_t *getLoop();

    void closeCb(uv_handle_t *handle);
};

template <typename T>
class HandleT : public Handle {
    friend class Process;

protected:
    using Handle::Handle;

    T *get() { return Handle::get<T>(); }
};

} // namespace uvxx

#endif // !UVXX_UVOBJECT_H
