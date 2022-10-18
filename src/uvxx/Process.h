#ifndef UVXX_PROCESS_H
#define UVXX_PROCESS_H

#include "Handle.h"

namespace uvxx {

class Stream;

class Process : public HandleT<uv_process_t> {
public:
    explicit Process(const std::shared_ptr<Loop> &loop, const std::string &file = "");
    ~Process() = default;

    std::string file;
    std::vector<std::string> args;
    uv_process_flags flags;

    bool spawn();
    int pid();
    bool kill(int signum);

    void onExit(const std::function<void(int64_t exit_status, int term_signal)> &cb) {
        exitCb_ = cb;
    };

    void setStdio(uint idx, uv_stdio_flags flags);
    void setStdio(uint idx, uv_stdio_flags flags, const std::shared_ptr<Stream> &stream);
    void setStdio(uint idx, uv_stdio_flags flags, int fd);
    void setStdin(uv_stdio_flags flags);
    void setStdin(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream);
    void setStdin(uv_stdio_flags flags, int fd);
    void setStdout(uv_stdio_flags flags);
    void setStdout(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream);
    void setStdout(uv_stdio_flags flags, int fd);
    void setStderr(uv_stdio_flags flags);
    void setStderr(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream);
    void setStderr(uv_stdio_flags flags, int fd);

    int addStdio(uv_stdio_flags flags);
    int addStdio(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream);
    int addStdio(uv_stdio_flags flags, int fd);

private:
    std::function<void(int64_t exit_status, int term_signal)> exitCb_;
    std::vector<uv_stdio_container_t> stdioContainers_;

    void exitCb(uv_process_t *handle, int64_t exit_status, int term_signal);
};

} // namespace uvxx

#endif // !UVXX_PROCESS_H
