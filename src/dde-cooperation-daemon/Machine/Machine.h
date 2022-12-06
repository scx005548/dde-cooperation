#ifndef MACHINE_MACHINE_H
#define MACHINE_MACHINE_H

#include <filesystem>

#include <QVector>
#include <QString>
#include <QObject>
#include <QtDBus>

#include "common.h"

#include "protocol/message.pb.h"

namespace uvxx {
class Loop;
class Timer;
class Async;
class Buffer;
} // namespace uvxx

class QTcpSocket;

class Manager;
class MachineDBusAdaptor;
class ClipboardBase;
class Request;
class InputEmittorWrapper;
class FuseServer;
class FuseClient;
class ConfirmDialogWrapper;

class Machine : public QObject, public std::enable_shared_from_this<Machine> {
    friend class Manager;
    friend class MachineDBusAdaptor;

    Q_OBJECT

public:
    Machine(Manager *manager,
            ClipboardBase *clipboard,
            const std::shared_ptr<uvxx::Loop> &uvLoop,
            QDBusConnection bus,
            uint32_t id,
            const std::filesystem::path &dataDir,
            const std::string &ip,
            uint16_t port,
            const DeviceInfo &sp);
    virtual ~Machine();

    const QString &path() const;
    const std::string &ip() const { return m_ip; };

    void updateMachineInfo(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);

    void receivedPing();
    void onPair(QTcpSocket *socket);
    void onInputGrabberEvent(uint8_t deviceType, unsigned int type, unsigned int code, int value);
    void onClipboardTargetsChanged(const std::vector<std::string> &targets);

    void flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept;
    void readTarget(const std::string &target);

    bool isPcMachine() const;
    bool isAndroid() const;

    bool connected() const { return !!m_conn; }

    virtual void handleConnected() = 0;
    virtual void handleDisconnected() = 0;

protected:
    void sendServiceStatusNotification();

private:
    QDBusConnection m_bus;
    Manager *m_manager;
    MachineDBusAdaptor *m_dbusAdaptor;
    ClipboardBase *m_clipboard;

    const std::filesystem::path m_dataDir;
    const std::filesystem::path m_mountpoint;

    uint16_t m_port;
    std::string m_uuid;
    std::string m_name;
    bool m_connected;
    DeviceOS m_os;
    Compositor m_compositor;
    bool m_deviceSharing;
    uint16_t m_direction;
    bool m_sharedClipboard = false;

    std::shared_ptr<uvxx::Timer> m_pingTimer;
    std::shared_ptr<uvxx::Timer> m_offlineTimer;
    std::unique_ptr<ConfirmDialogWrapper> m_confirmDialog;

    std::unordered_map<InputDeviceType, std::unique_ptr<InputEmittorWrapper>> m_inputEmittors;

    std::unique_ptr<FuseServer> m_fuseServer;
    std::unique_ptr<FuseClient> m_fuseClient;

    bool m_mounted;

    void ping();
    void onOffline();

    void initConnection();

    void handleDisconnectedAux();
    void dispatcher() noexcept;
    void handlePairResponseAux(const PairResponse &resp);
    void handleServiceOnOffMsg(const ServiceOnOffNotification &notification);
    void handleDeviceSharingStartRequest();
    void handleDeviceSharingStartResponse(const DeviceSharingStartResponse &resp);
    void handleDeviceSharingStopRequest();
    void handleInputEventRequest(const InputEventRequest &req);
    void handleFlowDirectionNtf(const FlowDirectionNtf &ntf);
    void handleFlowRequest(const FlowRequest &req);
    void handleFsRequest(const FsRequest &req);
    void handleFsResponse(const FsResponse &resp);
    void handleFsSendFileRequest(const FsSendFileRequest &req);
    void handleClipboardNotify(const ClipboardNotify &notify);
    void handleClipboardGetContentRequest(const ClipboardGetContentRequest &req);
    void handleClipboardGetContentResponse(const ClipboardGetContentResponse &resp);

    void stopDeviceSharingAux();
    void receivedUserConfirm(uvxx::Buffer &buff);
    void sendFlowDirectionNtf();
    void sendReceivedFilesSystemNtf(const std::string &path, bool isSuccess);

protected:
    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
    QTcpSocket *m_conn;

    std::string m_ip;

    void connect();
    void disconnect();
    void requestDeviceSharing();
    void stopDeviceSharing();
    void setFlowDirection(FlowDirection direction);
    void sendFiles(const QStringList &filePaths);
    void sendMessage(const Message &msg);
};

#endif // !MACHINE_MACHINE_H
