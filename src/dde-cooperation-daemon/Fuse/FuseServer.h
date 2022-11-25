#ifndef FUSE_FUSESERVER_H
#define FUSE_FUSESERVER_H

#include <filesystem>
#include <thread>

#include "protocol/fs.pb.h"

namespace uvxx {
class Loop;
class TCP;
class Buffer;
} // namespace uvxx

class Machine;

class FuseServer {
public:
    explicit FuseServer(const std::weak_ptr<Machine> &machine,
                        const std::shared_ptr<uvxx::Loop> &uvLoop);
    ~FuseServer();

    uint16_t port() const;

private:
    std::weak_ptr<Machine> m_machine;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::TCP> m_listen;
    std::shared_ptr<uvxx::TCP> m_conn;

    void handleNewConnection(bool) noexcept;
    void handleDisconnected() noexcept;
    void handleRequest(uvxx::Buffer &buff) noexcept;

    void methodGetattr(const FsMethodGetAttrRequest &req, FsMethodGetAttrResponse *resp);
    void methodOpen(const FsMethodOpenRequest &req, FsMethodOpenResponse *resp);
    void methodRead(const FsMethodReadRequest &req, FsMethodReadResponse *resp);
    void methodRelease(const FsMethodReleaseRequest &req, FsMethodReleaseResponse *resp);
    void methodReaddir(const FsMethodReadDirRequest &req, FsMethodReadDirResponse *resp);
};

#endif // !FUSE_FUSESERVER_H
