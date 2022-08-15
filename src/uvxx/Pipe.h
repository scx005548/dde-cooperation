#ifndef UVXX_PIPE_H
#define UVXX_PIPE_H

#include "Stream.h"

namespace uvxx {

class Pipe : public StreamT<Pipe, uv_pipe_t> {
public:
    explicit Pipe(const std::shared_ptr<Loop> &loop, bool ipc);
    ~Pipe() = default;

    bool open(uv_file file);

    void onConnected(const std::function<void()> &cb) { connectedCb_ = cb; }
    void onConnectFailed(
        const std::function<void(const std::string &title, const std::string &msg)> &cb) {
        connectFailedCb_ = cb;
    }

private:
    std::function<void()> connectedCb_{nullFunc{}};
    std::function<void(const std::string &title, const std::string &msg)> connectFailedCb_{
        nullFunc{}};
};

} // namespace uvxx

#endif // !UVXX_PIPE_H
