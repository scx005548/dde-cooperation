#include "FileReceiver.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <spdlog/spdlog.h>

#include "utils/sha256.h"

TransferResponse FileReceiver::parseRequest(TransferRequest request) noexcept {
    auto sock = Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                    Gio::SocketType::SOCKET_TYPE_STREAM,
                                    Gio::SocketProtocol::SOCKET_PROTOCOL_TCP);
    auto addr = Net::makeSocketAddress("0.0.0.0", 0);
    sock->bind(addr, true);
    sock->listen();

    auto local = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        sock->get_local_address());
    Glib::Thread::create([this, sock]() { m_recv(sock); }, false);

    TransferResponse response;
    response.set_key(START_TRANSFER_KEY);
    response.set_port(local->get_port());
    return response;
}

int32_t FileReceiver::m_allocId(const Glib::ustring &filename, int32_t parent) noexcept {
    int32_t id = static_cast<int32_t>(m_id.size() + 1);
    FileNode node;
    node.id = id;
    node.parent = parent;
    node.name = filename;
    m_id.push(id);
    m_idMap[id] = node;
    SPDLOG_INFO("alloc ID {}:{}", id, filename.c_str());

    return id;
}

void FileReceiver::m_releseId() noexcept {
    int32_t id = m_id.top();
    m_id.pop();
    m_idMap.erase(id);

    SPDLOG_INFO("release ID {}", id);
}

Glib::ustring FileReceiver::m_getFilePath(int32_t id) const noexcept {
    FileNode node = m_idMap.at(id);
    Glib::ustring path = node.name;
    while (node.parent > 0) {
        node = m_idMap.at(node.parent);
        path = Glib::ustring::compose("%1/%2", node.name, path);
    }

    return path;
}

void FileReceiver::m_recv(const Glib::RefPtr<Gio::Socket> &server) noexcept {
    auto sock = server->accept();
    sock->set_blocking(true);
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        sock->get_remote_address());
    SPDLOG_INFO("connected by {}:{}",
                remote->get_address()->to_string().c_str(),
                remote->get_port());

    while (true) {
        auto base = Message::recv_message_header(sock);
        switch (base.type()) {
        case SendFileRequestType:
            m_recvFile(sock, base);
            break;

        case SendDirRequestType:
            m_recvDir(sock, base);
            break;

        case StopTransferRequestType:
            goto END_RECV;

        default:
            SPDLOG_ERROR("unknow message type");
        }
    }

END_RECV:
    return;
}

void FileReceiver::m_recvFile(const Glib::RefPtr<Gio::Socket> &sock,
                              const BaseMessage &base) noexcept {
    int32_t id;
    Glib::ustring fileBaseName;
    {
        auto request = Message::recv_message_body<SendFileRequest>(sock, base);
        id = m_allocId(request.file_name(), request.parent_id());
        fileBaseName = request.file_name();
        SendFileResponse response;
        response.set_file_name(fileBaseName);
        response.set_file_id(id);
        Message::send_message(sock, SendFileRequestType, response);
    }

    Hash::Sha256 sha256;
    Hash::sha256Reset(&sha256);

    Glib::ustring fileSubPath = m_getFilePath(id);
    Glib::ustring path = Glib::ustring::compose("%1/%2", m_saveDir, fileSubPath);
    SPDLOG_INFO("save {}", path.c_str());
    FILE *fp = fopen(path.c_str(), "wb");

    while (true) {
        auto base = Message::recv_message_header(sock);
        if (base.type() == MessageType::SendFileBlockRequestType) {
            SPDLOG_INFO("recv SendFileBlockRequest");
            auto request = Message::recv_message_body<SendFileBlockRequest>(sock, base);
            size_t len = fwrite(request.block_data().data(), 1, request.block_size(), fp);
            Hash::sha256Update(&sha256, request.block_data().data(), request.block_size());

            SendFileBlockResponse response;
            response.set_file_id(id);
            response.set_block_serial(request.block_serial());
            Message::send_message(sock, MessageType::SendFileBlockResponseType, response);
            continue;
        }

        if (base.type() == MessageType::StopSendFileRequestType) {
            SPDLOG_INFO("recv StopSendFileRequest");
            auto request = Message::recv_message_body<StopSendFileRequest>(sock, base);
            Glib::ustring hex = Hash::sha256Hex(&sha256);
            if (hex != request.file_sha256()) {
                SPDLOG_ERROR("SHA256 mismatch {} != {}",
                             request.file_sha256().c_str(),
                             hex.c_str());
            }
            StopSendFileResponse response;
            response.set_file_name(fileBaseName);
            response.set_file_sha256(hex);
            response.set_correct(hex == request.file_sha256());
            Message::send_message(sock, MessageType::StopSendFileResponseType, response);
            break;
        }
    }

    fclose(fp);
}

void FileReceiver::m_recvDir(const Glib::RefPtr<Gio::Socket> &sock,
                             const BaseMessage &base) noexcept {
    auto request = Message::recv_message_body<SendDirRequest>(sock, base);
    uint32_t id = m_allocId(request.dir_name(), request.parent_id());
    SendDirResponse response;
    response.set_dir_name(request.dir_name());
    response.set_dir_id(id);
    Message::send_message(sock, SendFileRequestType, response);

    Glib::ustring fileSubPath = m_getFilePath(id);
    Glib::ustring path = Glib::ustring::compose("%1/%2", m_saveDir, fileSubPath);
    mkdir(path.c_str(), 0777);
    SPDLOG_INFO("mkdir {}", path.c_str());
}
