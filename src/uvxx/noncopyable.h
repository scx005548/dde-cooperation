#ifndef UVXX_NOCOPYABLE_H
#define UVXX_NOCOPYABLE_H

class noncopyable {
public:
    noncopyable() = default;
    ~noncopyable() = default;

protected:
    noncopyable(const noncopyable &) = delete;
    const noncopyable &operator=(const noncopyable &) = delete;

    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable &&) = default;
};

#endif // !UVXX_NOCOPYABLE_H
