#ifndef SENDTRANSFER_H
#define SENDTRANSFER_H

#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#include <QObject>
#include <QStringList>

class QStringList;
class QTcpServer;
class QTcpSocket;
class ObjectSendTransfer;

class SendTransfer : public QObject {
    Q_OBJECT

public:
    SendTransfer(const QStringList &filePaths);

    uint16_t receive();
    void send(const std::string &ip, uint16_t port);

private:
    QTcpSocket *m_conn;
    QStringList m_filePaths;
    std::shared_ptr<ObjectSendTransfer> m_objectSendTransfer;

    void dispatcher();
    void sendNextObject();
    void handleDisconnected();
};

#endif // !SENDTRANSFER_H
