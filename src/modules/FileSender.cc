#include "FileSender.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <spdlog/spdlog.h>

#include "utils/sha256.h"

void FileSender::setPilot(Glib::RefPtr<Gio::Socket> &sock) noexcept {
    m_pilot = sock;
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(
        m_pilot->get_remote_address());
    m_remoteIp = remote->get_address()->to_string();
}

void FileSender::pushFile(const Glib::ustring &file) noexcept {
    m_files.push(file);
}

Glib::ustring FileSender::popFile() noexcept {
    auto file = m_files.front();
    m_files.pop();
    return file;
}

TransferRequest FileSender::makeTransferRequest() noexcept {
    TransferRequest request;
    request.set_key(START_TRANSFER_KEY);
    return request;
}

StopTransferRequest FileSender::makeStopTransferRequest() noexcept {
    StopTransferRequest request;
    request.set_key(STOP_TRANSFER_KEY);
    return request;
}

SendDirRequest FileSender::makeSendDirRequest(const Glib::ustring &file, int32_t parent) noexcept {
    SendDirRequest request;
    request.set_dir_name(Gio::File::create_for_path(file)->get_basename());
    request.set_parent_id(parent);
    return request;
}

SendFileRequest FileSender::makeSendFileRequest(const Glib::ustring &file,
                                                int32_t parent) noexcept {
    SendFileRequest request;
    request.set_file_name(Gio::File::create_for_path(file)->get_basename());
    request.set_parent_id(parent);
    return request;
}

void FileSender::parseResponse(TransferResponse response) noexcept {
    auto sock = Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4,
                                    Gio::SocketType::SOCKET_TYPE_STREAM,
                                    Gio::SocketProtocol::SOCKET_PROTOCOL_TCP);

    auto addr = Net::makeSocketAddress(m_remoteIp, response.port());
    try {
        sock->connect(addr);
    } catch (Gio::Error &e) {
        SPDLOG_ERROR("{}", e.what().c_str());
    }

    Glib::Thread::create([this, sock]() { m_send(sock); }, false);
}

void FileSender::m_send(const Glib::RefPtr<Gio::Socket> &sock) noexcept {
    auto path = popFile();

    if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR)) {
        m_sendDir(sock, path);
    } else {
        m_sendFile(sock, path);
    }

    StopTransferRequest request;
    Message::send_message(sock, StopTransferRequestType, request);
}

void FileSender::m_sendFile(const Glib::RefPtr<Gio::Socket> &sock,
                            const Glib::ustring path,
                            int32_t parent) noexcept {
    int32_t id;
    {
        auto request = makeSendFileRequest(path, parent);
        Message::send_message(sock, SendFileRequestType, request);
        auto response = Message::recv_message<SendFileResponse>(sock);
        id = response.file_id();
    }

    char buff[BUFSIZ];
    FILE *fp = fopen(path.c_str(), "rb");

    Hash::Sha256 sha256;
    Hash::sha256Reset(&sha256);

    int64_t serial = 1;
    int64_t pos = 0;

    while (true) {
        size_t len = fread(buff, 1, BUFSIZ, fp);
        if (len == 0) {
            break;
        }

        SendFileBlockRequest request;
        request.set_file_id(id);
        request.set_block_serial(serial);
        request.set_block_pos(pos);
        request.set_block_size(len);
        request.set_block_data(buff, len);

        Message::send_message(sock, MessageType::SendFileBlockRequestType, request);
        Hash::sha256Update(&sha256, buff, len);

        serial += 1;
        pos += len;

        auto response = Message::recv_message<SendFileBlockResponse>(sock);
    }

    Glib::ustring hex = Hash::sha256Hex(&sha256);
    SPDLOG_INFO("SHA256: {}", Hash::sha256Hex(&sha256));

    StopSendFileRequest request;
    request.set_file_id(id);
    request.set_file_sha256(hex);
    Message::send_message(sock, MessageType::StopSendFileRequestType, request);

    auto response = Message::recv_message<StopSendFileResponse>(sock);
    if (!response.correct()) {
        SPDLOG_ERROR("SHA256 mismatch {} != {}", response.file_sha256().c_str(), hex.c_str());
    }

    fclose(fp);
}

void FileSender::m_sendDir(const Glib::RefPtr<Gio::Socket> &sock,
                           const Glib::ustring path,
                           int32_t parent) noexcept {
    int32_t id;
    {
        auto request = makeSendDirRequest(path, parent);
        Message::send_message(sock, SendDirRequestType, request);
        auto response = Message::recv_message<SendDirResponse>(sock);
        id = response.dir_id();
    }

    Glib::Dir dir(path);
    for (auto entry = dir.begin(); entry != dir.end(); entry++) {
        auto subFilePath = Glib::ustring::compose("%1/%2", path, (*entry));
        if (Glib::file_test(subFilePath, Glib::FILE_TEST_IS_DIR)) {
            m_sendDir(sock, subFilePath, id);
        } else {
            m_sendFile(sock, subFilePath, id);
        }
    }
}
