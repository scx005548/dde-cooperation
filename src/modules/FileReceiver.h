#ifndef FILE_RECIEVER_H
#define FILE_RECIEVER_H

#include <map>
#include <stack>
#include <queue>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include <dbus.h>

#include <send_file.pb.h>

#include <log.hpp>
#include <net.hpp>
#include <message.hpp>

#define START_TRANSFER_KEY "START-FILE-TRANSFER"
#define STOP_TRANSFER_KEY "STOP-FILE-TRANSFER"

class FileReceiver
{
public:
    TransferResponse parseRequest(TransferRequest request) noexcept;

private:
    struct FileNode
    {
        int32_t id;
        int32_t parent;
        Glib::ustring name;
    };

    // TODO: 可配置接收路径
    Glib::ustring m_saveDir = "/tmp"; 

    // 控制连接
    Glib::RefPtr<Gio::Socket> m_pilot; 

    // 接收端ID及其映射
    std::stack<int32_t> m_id;
    std::map<int32_t, FileNode> m_idMap;

    // 分配和释放ID
    int32_t m_allocId(const Glib::ustring& filename, int32_t parent) noexcept;
    void m_releseId() noexcept;
    Glib::ustring m_getFilePath(int32_t id) const noexcept;

    // 接收文件
    void m_recv(const Glib::RefPtr<Gio::Socket>& server) noexcept;
    void m_recvFile(const Glib::RefPtr<Gio::Socket>& sock, const BaseMessage &base) noexcept;
    void m_recvDir(const Glib::RefPtr<Gio::Socket>& sock, const BaseMessage &base) noexcept;
};

#endif // FILE_RECIEVER_H