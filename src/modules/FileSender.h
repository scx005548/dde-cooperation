#ifndef FILE_SENDER_H
#define FILE_SENDER_H

#include <map>
#include <stack>
#include <queue>

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include "dbus/dbus.h"
#include "utils/net.h"
#include "utils/message.h"

#include "protocol/send_file.pb.h"

#define START_TRANSFER_KEY "START-FILE-TRANSFER"
#define STOP_TRANSFER_KEY "STOP-FILE-TRANSFER"

class FileSender {
public:
    // 设置控制连接
    void setPilot(Glib::RefPtr<Gio::Socket> &sock) noexcept;

    // 设在远端IP
    void setRemoteIp(const Glib::ustring &ip) noexcept;

    // 发送文件队列
    void pushFile(const Glib::ustring &file) noexcept;
    Glib::ustring popFile() noexcept;

    // 创建请求
    static TransferRequest makeTransferRequest() noexcept;
    static StopTransferRequest makeStopTransferRequest() noexcept;
    static SendDirRequest makeSendDirRequest(const Glib::ustring &file,
                                             int32_t parent = 0) noexcept;
    static SendFileRequest makeSendFileRequest(const Glib::ustring &file,
                                               int32_t parent = 0) noexcept;

    // 解析并处理 Response
    void parseResponse(TransferResponse response) noexcept;

private:
    // 控制连接
    Glib::RefPtr<Gio::Socket> m_pilot;
    Glib::ustring m_remoteIp;

    // 发送队列
    std::queue<Glib::ustring> m_files;

    void m_send(const Glib::RefPtr<Gio::Socket> &sock) noexcept;
    void m_sendFile(const Glib::RefPtr<Gio::Socket> &sock,
                    const Glib::ustring path,
                    int32_t parent = 0) noexcept;
    void m_sendDir(const Glib::RefPtr<Gio::Socket> &sock,
                   const Glib::ustring path,
                   int32_t parent = 0) noexcept;
};

#endif // !FILE_SENDER_H
