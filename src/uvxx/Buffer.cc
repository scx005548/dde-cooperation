#include "Buffer.h"

using namespace uvxx;

void Buffer::retrieve(size_type n) {
    assert(size() >= n);
    if (size() == n) {
        clear();
    } else {
        gapSize_ += n;
        size_ -= n;
    }
}

void Buffer::retrieveAll() {
    clear();
}

void Buffer::clear() {
    gapSize_ = 0;
    size_ = 0;
}

void Buffer::expand(size_type n) {
    // 可写空间充足，不需要再分配内存
    size_type writable_size = writableSize();
    if (writable_size >= n) {
        return;
    }

    // 可写空间不足，但总空间够
    if (writable_size + gapSize_ >= n) {
        std::copy(begin(), end(), data_.begin());
        gapSize_ = 0;
        return;
    }

    // 剩余空间不足，重新分配空间
    container tmp;
    tmp.resize((size_ + n) * 2);
    std::copy(begin(), end(), tmp.begin());
    std::swap(data_, tmp);
    gapSize_ = 0;
}
