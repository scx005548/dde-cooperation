#include "Process.h"

#include "Stream.h"

using namespace uvxx;

Process::Process(const std::shared_ptr<Loop> &loop, const std::string &p)
    : HandleT(loop)
    , file(p)
    , flags(static_cast<uv_process_flags>(0))
    , stdioContainers_({{UV_IGNORE, {nullptr}}, {UV_IGNORE, {nullptr}}, {UV_IGNORE, {nullptr}}}) {
}

bool Process::spawn() {
    std::vector<char *> argsA(args.size() + 2);
    argsA[0] = file.data();
    for (size_t i = 0; i < args.size(); i++) {
        argsA[i + 1] = args[i].data();
    }
    argsA[argsA.size() - 1] = nullptr;

    uv_process_options_t options{};
    options.file = file.c_str();
    options.args = argsA.data();
    options.stdio = stdioContainers_.data();
    options.stdio_count = stdioContainers_.size();
    options.exit_cb = CallbackWrapper<&Process::exitCb>::func;
    options.flags = flags;

    return initialize(&uv_spawn, const_cast<const uv_process_options_t *>(&options));
}

int Process::pid() {
    return get()->pid;
}

bool Process::kill(int signum) {
    return invoke(&uv_process_kill, get(), signum);
}

void Process::setStdio(uint idx, uv_stdio_flags flags) {
    stdioContainers_[idx] = uv_stdio_container_t{flags, {nullptr}};
}

void Process::setStdio(uint idx, uv_stdio_flags flags, const std::shared_ptr<Stream> &stream) {
    stdioContainers_[idx] = uv_stdio_container_t{flags, {stream->get()}};
}

void Process::setStdio(uint idx, uv_stdio_flags flags, int fd) {
    stdioContainers_[idx] = uv_stdio_container_t{flags, {.fd = fd}};
}

void Process::setStdin(uv_stdio_flags flags) {
    setStdio(0, flags);
}

void Process::setStdin(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream) {
    setStdio(0, flags, stream);
}

void Process::setStdin(uv_stdio_flags flags, int fd) {
    setStdio(0, flags, fd);
}

void Process::setStdout(uv_stdio_flags flags) {
    setStdio(1, flags);
}

void Process::setStdout(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream) {
    setStdio(1, flags, stream);
}

void Process::setStdout(uv_stdio_flags flags, int fd) {
    setStdio(1, flags, fd);
}

void Process::setStderr(uv_stdio_flags flags) {
    setStdio(2, flags);
}

void Process::setStderr(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream) {
    setStdio(2, flags, stream);
}

void Process::setStderr(uv_stdio_flags flags, int fd) {
    setStdio(2, flags, fd);
}

int Process::addStdio(uv_stdio_flags flags) {
    stdioContainers_.emplace_back(uv_stdio_container_t{flags, {nullptr}});
    return stdioContainers_.size() - 1;
}

int Process::addStdio(uv_stdio_flags flags, const std::shared_ptr<Stream> &stream) {
    stdioContainers_.emplace_back(uv_stdio_container_t{flags, {stream->get()}});
    return stdioContainers_.size() - 1;
}

int Process::addStdio(uv_stdio_flags flags, int fd) {
    stdioContainers_.emplace_back(uv_stdio_container_t{flags, {.fd = fd}});
    return stdioContainers_.size() - 1;
}

void Process::exitCb([[maybe_unused]] uv_process_t *handle, int64_t exit_status, int term_signal) {
    spdlog::debug("process {} {} [{}] exited with status {}", file, fmt::join(args, ", "), pid(), exit_status);
    exitCb_(exit_status, term_signal);
}
