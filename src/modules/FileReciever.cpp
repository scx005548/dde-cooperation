#include <FileReceiver.h>
#include <sha256.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

TransferResponse FileReceiver::parseRequest(TransferRequest request) noexcept
{
    auto sock = Gio::Socket::create(Gio::SocketFamily::SOCKET_FAMILY_IPV4, 
                                Gio::SocketType::SOCKET_TYPE_STREAM, 
                                Gio::SocketProtocol::SOCKET_PROTOCOL_TCP);
    auto addr = Net::makeSocketAddress("0.0.0.0", 0);
    sock->bind(addr, true);
    sock->listen();

    auto local = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(sock->get_local_address());
    Glib::Thread::create([this, sock](){
        m_recv(sock);
    }, false);

    TransferResponse response;
    response.set_key(START_TRANSFER_KEY);
    response.set_port(local->get_port());
    return response;
}

int32_t FileReceiver::m_allocId(const Glib::ustring& filename, int32_t parent) noexcept
{
    int32_t id = static_cast<int32_t>(m_id.size() + 1);
    FileNode node;
    node.id = id;
    node.parent = parent;
    node.name = filename;
    m_id.push(id);
    m_idMap[id] = node;
    INFO("alloc ID %d:%s\n", id, filename.c_str());

    return id;
}

void FileReceiver::m_releseId() noexcept
{
    int32_t id = m_id.top();
    m_id.pop();
    m_idMap.erase(id);

    INFO("release ID %d\n", id);
}

Glib::ustring FileReceiver::m_getFilePath(int32_t id) const noexcept
{
    FileNode node = m_idMap.at(id);
    Glib::ustring path = node.name;
    while(node.parent > 0)
    {
        node = m_idMap.at(node.parent);
        path = Glib::ustring::compose("%1/%2", node.name, path);
    }

    return path;
}

void FileReceiver::m_recv(const Glib::RefPtr<Gio::Socket>& server) noexcept
{
    auto sock = server->accept();
    sock->set_blocking(true);
    auto remote = Glib::RefPtr<Gio::InetSocketAddress>::cast_dynamic<Gio::SocketAddress>(sock->get_remote_address());
    INFO("connected by %s:%d\n", remote->get_address()->to_string().c_str(), remote->get_port());

    while(true)
    {
        auto base = Message::recv_message_header(sock);
        switch (base.type())
        {
        case SendFileRequestType:
            m_recvFile(sock, base);
            break;

        case SendDirRequestType:
            m_recvDir(sock, base);
            break;

        case StopTransferRequestType:
            goto END_RECV;

        default:
            ERROR("unknow message type\n");
        }
    }

END_RECV:
    return;
}

void FileReceiver::m_recvFile(const Glib::RefPtr<Gio::Socket>& sock, const BaseMessage &base) noexcept
{
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
    INFO("save %s\n", path.c_str());
    FILE* fp = fopen(path.c_str(), "wb");

    while (true)
    {
        auto base = Message::recv_message_header(sock);
        if (base.type() == MessageType::SendFileBlockRequestType)
        {
            INFO("recv SendFileBlockRequest\n");
            auto request = Message::recv_message_body<SendFileBlockRequest>(sock, base);
            size_t len = fwrite(request.block_data().data(), 1, request.block_size(), fp);
            Hash::sha256Update(&sha256, request.block_data().data(), request.block_size());

            SendFileBlockResponse response;
            response.set_file_id(id);
            response.set_block_serial(request.block_serial());
            Message::send_message(sock, MessageType::SendFileBlockResponseType, response);
            continue;
        }

        if (base.type() == MessageType::StopSendFileRequestType)
        {
            INFO("recv StopSendFileRequest\n");
            auto request = Message::recv_message_body<StopSendFileRequest>(sock, base);
            Glib::ustring hex = Hash::sha256Hex(&sha256);
            if (hex != request.file_sha256())
            {
                ERROR("SHA256 mismatch %s != %s\n", request.file_sha256().c_str(), hex.c_str());
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

void FileReceiver::m_recvDir(const Glib::RefPtr<Gio::Socket>& sock, const BaseMessage &base) noexcept
{
    auto request = Message::recv_message_body<SendDirRequest>(sock, base);
    uint32_t id = m_allocId(request.dir_name(), request.parent_id());
    SendDirResponse response;
    response.set_dir_name(request.dir_name());
    response.set_dir_id(id);
    Message::send_message(sock, SendFileRequestType, response);

    Glib::ustring fileSubPath = m_getFilePath(id);
    Glib::ustring path = Glib::ustring::compose("%1/%2", m_saveDir, fileSubPath);
    mkdir(path.c_str(), 0777);
    INFO("mkdir %s\n", path.c_str());
}