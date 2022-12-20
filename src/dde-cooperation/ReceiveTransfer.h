#ifndef RECEIVETRANSFER_H
#define RECEIVETRANSFER_H

#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <fstream>

#include <QObject>

#include "protocol/message.pb.h"

class QTcpServer;
class QTcpSocket;

class ReceiveTransfer : public QObject {
    Q_OBJECT

public:
    ReceiveTransfer(const std::vector<std::string> &filePaths, const std::filesystem::path &dest);

    uint16_t receive();

private:
    QTcpServer *m_listen;
    QTcpSocket *m_conn;
    std::vector<std::string> m_filePaths;
    std::filesystem::path m_dest;
    std::unordered_map<std::string, std::ofstream> m_streams;

    std::filesystem::path getPath(const std::string &relpath);

    void handleNewConnection();
    void handleDisconnected();
    void dispatcher();
    void handleSendFileRequest(const SendFileRequest &req);
    void handleStopSendFileRequest(const StopSendFileRequest &req);
    void handleSendFileChunkRequest(const SendFileChunkRequest &req);
    void handleSendDirRequest(const SendDirRequest &req);
    void handleStopTransferRequest(const StopTransferRequest &req);
};

#endif // !RECEIVETRANSFER_H
