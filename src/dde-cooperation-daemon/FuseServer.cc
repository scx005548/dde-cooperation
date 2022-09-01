#include "FuseServer.h"

#include <filesystem>
#include <algorithm>

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <google/protobuf/util/time_util.h>

#include "utils/net.h"
#include "utils/message_helper.h"
#include "protocol/message.pb.h"

#include "Machine.h"

#include "uvxx/Loop.h"
#include "uvxx/TCP.h"
#include "uvxx/UDP.h"
#include "uvxx/Addr.h"

namespace fs = std::filesystem;

static constexpr size_t maxRead = 128 * 1024;

FuseServer::FuseServer(const std::weak_ptr<Machine> &machine,
                       const std::shared_ptr<uvxx::Loop> &uvLoop)
    : m_machine(machine)
    , m_uvLoop(uvLoop)
    , m_listen(std::make_shared<uvxx::TCP>(m_uvLoop)) {

    m_listen->bind("0.0.0.0");
    m_listen->listen();
    m_listen->onNewConnection(uvxx::memFunc(this, &FuseServer::handleNewConnection));
}

uint16_t FuseServer::port() const {
    return m_listen->localAddress()->ipv4()->port();
}

void FuseServer::handleNewConnection(bool) noexcept {
    if (m_conn) {
        return;
    }

    // TODO: check client
    m_conn = m_listen->accept();
    m_conn->onReceived(uvxx::memFunc(this, &FuseServer::handleRequest));
    m_conn->startRead();
    m_conn->onClosed(uvxx::memFunc(this, &FuseServer::handleDisconnected));
}

void FuseServer::handleDisconnected() noexcept {
}

void FuseServer::handleRequest(uvxx::Buffer &buff) noexcept {
    auto res = MessageHelper::parseMessage<Message>(buff);
    if (!res.has_value()) {
        return;
    }

    Message &msg = res.value();

    switch (msg.payload_case()) {
    case Message::PayloadCase::kFsMethodGetAttrRequest: {
        const auto &req = msg.fsmethodgetattrrequest();

        Message resp;
        methodGetattr(req, resp.mutable_fsmethodgetattrresponse());
        m_conn->write(MessageHelper::genMessage(resp));
        break;
    }
    case Message::PayloadCase::kFsMethodReadRequest: {
        const auto &req = msg.fsmethodreadrequest();

        Message resp;
        methodRead(req, resp.mutable_fsmethodreadresponse());
        m_conn->write(MessageHelper::genMessage(resp));
        break;
    }
    case Message::PayloadCase::kFsMethodReadDirRequest: {
        const auto &req = msg.fsmethodreaddirrequest();

        Message resp;
        methodReaddir(req, resp.mutable_fsmethodreaddirresponse());
        m_conn->write(MessageHelper::genMessage(resp));
        break;
    }
    case Message::PayloadCase::kFsMethodOpenRequest: {
        const auto &req = msg.fsmethodopenrequest();

        Message resp;
        methodOpen(req, resp.mutable_fsmethodopenresponse());
        m_conn->write(MessageHelper::genMessage(resp));
        break;
    }
    case Message::PayloadCase::kFsMethodReleaseRequest: {
        const auto &req = msg.fsmethodreleaserequest();

        Message resp;
        methodRelease(req, resp.mutable_fsmethodreleaseresponse());
        m_conn->write(MessageHelper::genMessage(resp));
        break;
    }
    default: {
        spdlog::warn("invalid message type: {}", msg.payload_case());
        m_conn->close();
        return;
    }
    }
}

void FuseServer::methodGetattr(const FsMethodGetAttrRequest &req, FsMethodGetAttrResponse *resp) {
    spdlog::info("methodGetattr: {}", req.path());

    struct stat st;
    int result = stat(req.path().c_str(), &st);

    resp->set_serial(req.serial());
    resp->set_result(result);
    resp->mutable_stat()->set_ino(st.st_ino);
    resp->mutable_stat()->set_nlink(st.st_nlink);
    resp->mutable_stat()->set_mode(st.st_mode);
    resp->mutable_stat()->set_uid(st.st_uid);
    resp->mutable_stat()->set_gid(st.st_gid);
    resp->mutable_stat()->set_size(st.st_size);
    resp->mutable_stat()->set_blksize(st.st_blksize);
    resp->mutable_stat()->set_blocks(st.st_blocks);

    resp->mutable_stat()->mutable_atime()->set_seconds(st.st_atim.tv_sec);
    resp->mutable_stat()->mutable_atime()->set_nanos(st.st_atim.tv_nsec);
    resp->mutable_stat()->mutable_mtime()->set_seconds(st.st_mtim.tv_sec);
    resp->mutable_stat()->mutable_mtime()->set_nanos(st.st_mtim.tv_nsec);
    resp->mutable_stat()->mutable_ctime()->set_seconds(st.st_ctim.tv_sec);
    resp->mutable_stat()->mutable_ctime()->set_nanos(st.st_ctim.tv_nsec);

    spdlog::info("path: {}, S_IFDIR: {}, S_IFREG: {}, nlink: {}",
                 req.path(),
                 static_cast<bool>(st.st_mode & S_IFDIR),
                 static_cast<bool>(st.st_mode & S_IFREG),
                 st.st_nlink);
}

void FuseServer::methodOpen(const FsMethodOpenRequest &req, FsMethodOpenResponse *resp) {
    spdlog::info("methodOpen: {}", req.path());

    int flags = 0;
    if (req.has_fi()) {
        flags = req.fi().flags();
    }

    int result = 0;
    int fd = open(req.path().c_str(), flags);
    if (fd == -1) {
        result = errno;
    }

    resp->set_result(result);
    resp->set_fh(fd);
}

void FuseServer::methodRead(const FsMethodReadRequest &req, FsMethodReadResponse *resp) {
    if (!req.has_fi()) {
        spdlog::error("methodRead: no fi");
        resp->set_result(-EBADF);
        return;
    }

    spdlog::info("methodRead: fh: {}", req.fi().fh());

    off_t off = lseek(req.fi().fh(), req.offset(), SEEK_SET);
    if (off == -1) {
        spdlog::error("lseek failed: {}({})", strerror(errno), errno);
        resp->set_result(-errno);
        return;
    }

    size_t size = req.size();
    if (size > maxRead) {
        size = maxRead;
    }

    resp->mutable_data()->resize(size);
    size = read(req.fi().fh(), resp->mutable_data()->data(), size);
    resp->mutable_data()->resize(size);
    resp->set_result(size);
}

void FuseServer::methodRelease(const FsMethodReleaseRequest &req, FsMethodReleaseResponse *resp) {
    if (!req.has_fi()) {
        spdlog::error("methodRelease: no fi");
        resp->set_result(EBADF);
        return;
    }

    spdlog::info("methodRelease: fh: {}", req.fi().fh());
    int result = close(req.fi().fh());
    resp->set_result(result);
}

void FuseServer::methodReaddir(const FsMethodReadDirRequest &req, FsMethodReadDirResponse *resp) {
    spdlog::info("methodReaddir: {}", req.path());
    resp->set_result(0);

    fs::path p = req.path();

    auto iter = fs::directory_iterator(p, fs::directory_options::skip_permission_denied);
    auto end_iter = fs::end(iter);
    auto ec = std::error_code();
    for (; iter != end_iter; iter.increment(ec)) {
        if (ec) {
            continue;
        }

        resp->mutable_item()->Add(iter->path().filename().string());
    }
}
