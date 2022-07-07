#ifndef UVXX_UVTYPE_H
#define UVXX_UVTYPE_H

#include <memory>

#include <spdlog/spdlog.h>

#include "noncopyable.h"

namespace uvxx {

template <typename U, typename T>
class UvType : public noncopyable {
protected:
    UvType()
        : uv_type_(std::make_shared<U>()) {
        get()->data = this;
    }

    template <typename D = T>
    D *get() {
        return reinterpret_cast<D *>(uv_type_.get());
    }

private:
    std::shared_ptr<U> uv_type_;
    std::shared_ptr<void> self_;
};

} // namespace uvxx

#endif // !UVXX_UVTYPE_H
