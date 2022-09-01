#ifndef UVXX_BUFFER_H
#define UVXX_BUFFER_H

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

#include "noncopyable.h"

namespace uvxx {

class Buffer : public noncopyable {
public:
    using value_type = char;
    using reference = char &;
    using const_reference = const char &;
    using const_pointer = const char *;
    using pointer = char *;
    using container = std::vector<value_type>;
    using iterator = typename container::iterator;
    using const_iterator = const iterator;
    using size_type = typename container::size_type;

public:
    Buffer()
        : data_()
        , gapSize_(0)
        , size_(0) {}
    explicit Buffer(size_type size)
        : data_(size)
        , gapSize_(0)
        , size_(0) {}

    size_type capacity() const { return data_.size(); }
    size_type gapSize() const { return gapSize_; }
    size_type size() const { return size_; }
    size_type writableSize() const { return data_.size() - gapSize_ - size_; }

    iterator begin() { return data_.begin() + gapSize_; }
    iterator end() { return data_.begin() + gapSize_ + size_; }
    const_iterator cbegin() { return begin(); }
    const_iterator cend() { return end(); }
    const_pointer data() { return &*(cbegin()); }

    pointer writable() { return &*(end()); }
    void writed(size_type size) { size_ += size; }

    template <typename T>
    T peak() {
        return *reinterpret_cast<T *>(&*begin());
    }

    void retrieve(size_type n);
    void retrieveAll();

    /**
     * @brief 清空
     */
    void clear();

    /**
     * @brief 扩容
     */
    void expand(size_type n);

private:
    container data_;
    size_t gapSize_;
    size_t size_;
};

} // namespace uvxx

#endif // !UVXX_BUFFER_H
