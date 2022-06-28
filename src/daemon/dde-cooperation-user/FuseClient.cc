#include "FuseClient.h"

#include <functional>
#include <filesystem>

#include <sys/time.h>

#include <spdlog/spdlog.h>
#include <google/protobuf/util/time_util.h>

#include "utils/message_helper.h"
#include "protocol/message.pb.h"

#include "uvxx/Loop.h"
#include "uvxx/Async.h"

namespace fs = std::filesystem;

template <auto F>
struct fuseOpsWrapper;

template <typename... Args, int (FuseClient::*F)(Args...)>
struct fuseOpsWrapper<F> {
    static int func(Args... args) {
        auto *ctx = fuse_get_context();
        auto *p = reinterpret_cast<FuseClient *>(ctx->private_data);
        return (p->*F)(args...);
    }
};

FuseClient::FuseClient(const std::string &ip,
                       uint16_t port,
                       const std::filesystem::path &mountpoint)
    : m_ip(ip)
    , m_port(port)
    , m_uvLoop(std::make_shared<uvxx::Loop>())
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop))
    , m_conn(std::make_shared<uvxx::TCP>(m_uvLoop))
    , m_mountpoint(mountpoint)
    , m_args(FUSE_ARGS_INIT(0, nullptr))
    , m_fuse(std::unique_ptr<fuse, decltype(&fuse_destroy)>(nullptr, &fuse_destroy)) {
    spdlog::info("FuseClient::FuseClient, mountpoint: {}", m_mountpoint.string());

    if (!fs::exists(m_mountpoint)) {
        fs::create_directories(m_mountpoint);
    }

    fuse_opt_add_arg(&m_args, m_mountpoint.c_str());
    fuse_opt_add_arg(&m_args, "-d");
    fuse_opt_add_arg(&m_args, "-odefault_permissions");

    m_conn->onConnected([this]() { m_mountThread = std::thread(&FuseClient::mount, this); });
    m_conn->onReceived(uvxx::memFunc(this, &FuseClient::handleResponse));
    m_conn->connect(m_ip, m_port);
    m_conn->startRead();

    m_uvThread = std::thread([this]() { m_uvLoop->run(); });
}

FuseClient::~FuseClient() {
    exit();
}

bool FuseClient::mount() {
    spdlog::info("FuseClient::mount");

#if __cplusplus < 202002L
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++20-designator"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#endif
    static fuse_operations ops{
        .getattr = fuseOpsWrapper<&FuseClient::getattr>::func,
        .open = fuseOpsWrapper<&FuseClient::open>::func,
        .read = fuseOpsWrapper<&FuseClient::read>::func,
        .release = fuseOpsWrapper<&FuseClient::release>::func,
        .readdir = fuseOpsWrapper<&FuseClient::readdir>::func,
    };
#if __cplusplus < 202002L
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    m_fuse.reset(fuse_new(&m_args, &ops, sizeof(ops), this));
    int ret = fuse_mount(m_fuse.get(), m_mountpoint.c_str());
    if (ret != 0) {
        return false;
    }

    ret = fuse_loop(m_fuse.get());
    fuse_unmount(m_fuse.get());
    if (ret != 0) {
        return false;
    }

    return true;
}

void FuseClient::exit() {
    if (m_fuse) {
        fuse_exit(m_fuse.get());
    }

    if (m_uvThread.joinable()) {
        m_async->wake([this]() { m_uvLoop->stop(); });

        m_uvThread.join();
    }
}

int FuseClient::getattr(const char *path,
                        struct stat *const st,
                        [[maybe_unused]] struct fuse_file_info *fi) {
    spdlog::debug("getattr: {}", path);

    Message msg;
    FsMethodGetAttrRequest *req = msg.mutable_fsmethodgetattrrequest();
    req->set_path(path);
    m_conn->write(MessageHelper::genMessage(msg));

    auto resp = std::static_pointer_cast<FsMethodGetAttrResponse>(waitForServerReply());
    auto retStat = resp->stat();

    memset(st, 0, sizeof(struct stat));
    st->st_ino = retStat.ino();
    st->st_nlink = retStat.nlink();
    st->st_mode = retStat.mode();
    st->st_uid = retStat.uid();
    st->st_gid = retStat.gid();
    st->st_size = retStat.size();
    st->st_blksize = retStat.blksize();
    st->st_blocks = retStat.blocks();
    st->st_atim.tv_sec = retStat.atime().seconds();
    st->st_atim.tv_nsec = retStat.atime().nanos();
    st->st_mtim.tv_sec = retStat.mtime().seconds();
    st->st_mtim.tv_nsec = retStat.mtime().nanos();
    st->st_ctim.tv_sec = retStat.ctime().seconds();
    st->st_ctim.tv_nsec = retStat.ctime().nanos();

    spdlog::info("path: {}, S_IFDIR: {}, S_IFREG: {}, nlink: {}",
                 path,
                 static_cast<bool>(st->st_mode & S_IFDIR),
                 static_cast<bool>(st->st_mode & S_IFREG),
                 st->st_nlink);

    return resp->result();
}

int FuseClient::open(const char *path, struct fuse_file_info *fi) {
    spdlog::debug("open: {}", path);

    Message msg;
    FsMethodOpenRequest *req = msg.mutable_fsmethodopenrequest();
    req->set_path(path);
    m_conn->write(MessageHelper::genMessage(msg));

    auto resp = std::static_pointer_cast<FsMethodOpenResponse>(waitForServerReply());
    if (resp->has_fh()) {
        fi->fh = resp->fh();
    }

    fi->direct_io = true;

    return resp->result();
}

int FuseClient::read(const char *path,
                     char *buf,
                     size_t size,
                     off_t offset,
                     struct fuse_file_info *fi) {
    spdlog::debug("read: {}, fh: {}, size: {}", path, fi->fh, size);

    Message msg;
    FsMethodReadRequest *req = msg.mutable_fsmethodreadrequest();
    req->set_offset(offset);
    req->set_size(size);
    auto rfi = req->mutable_fi();
    rfi->set_fh(fi->fh);
    m_conn->write(MessageHelper::genMessage(msg));

    auto resp = std::static_pointer_cast<FsMethodReadResponse>(waitForServerReply());
    spdlog::info("readed size: {}", resp->data().size());
    memcpy(buf, resp->data().data(), resp->data().size());

    return resp->result();
}

int FuseClient::release(const char *path, struct fuse_file_info *fi) {
    spdlog::debug("release: {}", path);
    Message msg;
    FsMethodReleaseRequest *req = msg.mutable_fsmethodreleaserequest();
    req->set_path(path);
    req->mutable_fi()->set_fh(fi->fh);
    m_conn->write(MessageHelper::genMessage(msg));

    auto resp = std::static_pointer_cast<FsMethodReleaseResponse>(waitForServerReply());

    return resp->result();
}

int FuseClient::readdir(const char *path,
                        void *buf,
                        fuse_fill_dir_t filler,
                        [[maybe_unused]] off_t offset,
                        [[maybe_unused]] struct fuse_file_info *fi,
                        [[maybe_unused]] enum fuse_readdir_flags flags) {
    spdlog::debug("readdir: {}", path);
    Message msg;
    FsMethodReadDirRequest *req = msg.mutable_fsmethodreaddirrequest();
    req->set_path(path);
    m_conn->write(MessageHelper::genMessage(msg));

    auto resp = std::static_pointer_cast<FsMethodReadDirResponse>(waitForServerReply());
    for (const std::string &i : resp->item()) {
        filler(buf, i.c_str(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return resp->result();
}

void FuseClient::handleResponse(std::shared_ptr<char[]> buffer,
                                [[maybe_unused]] ssize_t size) noexcept {
    auto buff = buffer.get();
    auto header = MessageHelper::parseMessageHeader(buff);
    buff += header_size;
    size -= header_size;
    Message msg = MessageHelper::parseMessageBody(buff, header.size);

    switch (msg.payload_case()) {
    case Message::PayloadCase::kFsMethodGetAttrResponse: {
        const auto &data = msg.fsmethodgetattrresponse();
        m_buff.reset(new FsMethodGetAttrResponse(data));
        m_cv.notify_one();
        break;
    }
    case Message::PayloadCase::kFsMethodOpenResponse: {
        const auto &data = msg.fsmethodopenresponse();
        m_buff.reset(new FsMethodOpenResponse(data));
        m_cv.notify_one();
        break;
    }
    case Message::PayloadCase::kFsMethodReadResponse: {
        const auto &data = msg.fsmethodreadresponse();
        m_buff.reset(new FsMethodReadResponse(data));
        m_cv.notify_one();
        break;
    }
    case Message::PayloadCase::kFsMethodReadDirResponse: {
        const auto &data = msg.fsmethodreaddirresponse();
        m_buff.reset(new FsMethodReadDirResponse(data));
        m_cv.notify_one();
        break;
    }
    case Message::PayloadCase::kFsMethodReleaseResponse: {
        const auto &data = msg.fsmethodreleaseresponse();
        m_buff.reset(new FsMethodReleaseResponse(data));
        m_cv.notify_one();
        break;
    }
    default: {
        spdlog::warn("invalid message type: {}", msg.payload_case());
        m_conn->close();
        return;
    }
    }
}

std::shared_ptr<google::protobuf::Message> FuseClient::waitForServerReply() {
    std::unique_lock lk(m_mut);
    m_cv.wait(lk);

    spdlog::info("responsed");

    return m_buff;
}
