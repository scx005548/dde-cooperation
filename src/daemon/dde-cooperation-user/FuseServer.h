#ifndef DDE_COOPERATION_USER_FUSE_SERVER_H
#define DDE_COOPERATION_USER_FUSE_SERVER_H

#include <filesystem>
#include <thread>

#include <glibmm.h>
#include <giomm.h>

#include "protocol/fs.pb.h"

namespace uvxx {
class Loop;
class TCP;
} // namespace uvxx

class FuseServer {
public:
    explicit FuseServer(const std::filesystem::path &path);

    uint16_t port() const;

private:
    const std::filesystem::path &m_path;

    std::thread m_uvThread;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::TCP> m_listen;
    std::shared_ptr<uvxx::TCP> m_conn;

    void handleConnect(bool) noexcept;
    void handleRequest(std::shared_ptr<char[]> buffer, ssize_t size) noexcept;

    void methodGetattr(const FsMethodGetAttrRequest &req, FsMethodGetAttrResponse *resp);
    void methodOpen(const FsMethodOpenRequest &req, FsMethodOpenResponse *resp);
    void methodRead(const FsMethodReadRequest &req, FsMethodReadResponse *resp);
    void methodRelease(const FsMethodReleaseRequest &req, FsMethodReleaseResponse *resp);
    void methodReaddir(const FsMethodReadDirRequest &req, FsMethodReadDirResponse *resp);
};

#endif // !DDE_COOPERATION_USER_FUSE_SERVER_H
