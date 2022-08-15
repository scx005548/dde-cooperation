#ifndef UVXX_UTILS_H
#define UVXX_UTILS_H

#include <spdlog/spdlog.h>

namespace uvxx {

class nullFunc {
public:
    template <class F>
    operator std::function<F>() {
        return [](auto &&...) {};
    }
};

template <class C, typename R, typename... Args>
std::function<void(Args...)> memFunc(C *p, R (C::*f)(Args...)) {
    return [p, f](Args &&...args) { return (p->*f)(std::forward<Args>(args)...); };
}

template <typename T>
inline constexpr bool always_false_v = false;

template <class T>
struct tag_t {
    using type = T;
};

template <class T>
using block_deduction = typename tag_t<T>::type;

template <auto F>
struct CallbackWrapper;
template <typename C, typename R, typename Handle, typename... Args, R (C::*F)(Handle, Args...)>
struct CallbackWrapper<F> {
    static R func(Handle handle, Args... args) {
        auto *p = static_cast<C *>(handle->data);
        return (p->*F)(handle, args...);
    }
};

} // namespace uvxx

#endif // !UVXX_UTILS_H
