#ifndef DDE_COOPERATION_USER_FUSE_H
#define DDE_COOPERATION_USER_FUSE_H

#include <filesystem>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include <google/protobuf/message.h>

#include <glibmm.h>
#include <giomm.h>

namespace uvxx {
class Loop;
class Async;
class TCP;
} // namespace uvxx

class FuseClient {
public:
    explicit FuseClient(const std::string &ip,
                        uint16_t port,
                        const std::filesystem::path &mountpoint);
    ~FuseClient();

    bool mount();
    void exit();

private:
    std::string m_ip;
    uint16_t m_port;
    std::thread m_uvThread;
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
    std::shared_ptr<uvxx::TCP> m_conn;

    const std::filesystem::path m_mountpoint;
    fuse_args m_args;
    std::unique_ptr<fuse, decltype(&fuse_destroy)> m_fuse;

    std::thread m_mountThread;
    std::mutex m_mut;
    std::condition_variable m_cv;
    std::shared_ptr<google::protobuf::Message> m_buff;

    int getattr(const char *path, struct stat *st, struct fuse_file_info *fi);
    int open(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offser, struct fuse_file_info *fi);
    int release(const char *path, struct fuse_file_info *fi);
    int readdir(const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

    void handleResponse(std::shared_ptr<char[]> buffer, ssize_t size) noexcept;
    std::shared_ptr<google::protobuf::Message> waitForServerReply();
};

#endif // !DDE_COOPERATION_USER_FUSE_H
