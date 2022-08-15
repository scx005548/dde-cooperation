#include "Pipe.h"

using namespace uvxx;

Pipe::Pipe(const std::shared_ptr<Loop> &loop, bool ipc)
    : StreamT(loop) {
    initialize(&uv_pipe_init, static_cast<int>(ipc));
}

bool Pipe::open(uv_file file) {
    return invoke(&uv_pipe_open, get(), file);
}
