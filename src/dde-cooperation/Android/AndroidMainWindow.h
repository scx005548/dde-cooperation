#ifndef ANDROID_ANDROIDMAINWINDOW_H
#define ANDROID_ANDROIDMAINWINDOW_H

#include <DMainWindow>
#include <QQmlApplicationEngine>

#include "AdbProcess.h"

class QQuickWidget;
class QrCodeProxy;
class DeviceProxy;
class VideoFrameProvider;

namespace qsc {
class IDevice;
}

class AndroidMainWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint16 stage MEMBER m_stage NOTIFY stageChanged)
    Q_PROPERTY(QString uuid MEMBER m_uuid)

public:
    enum STAGE {
        STAGE_NONE,
        STAGE_SHOW_QR_CODE,
        STAGE_OPEN_DEVELOPER,
        STAGE_OPEN_ADB,
        STAGE_OPEN_WIRELESS_DBG,
        STAGE_CONNECT_WIRELESS_DBG,
        STAGE_ALREADY,
    };

    AndroidMainWindow(const QString &uuid, QObject *parent = nullptr);
    ~AndroidMainWindow();

    void showConnectDevice();
    void setWirelessDbgAddress(const QString &ip, uint16_t port);
    void openTCPAdb();
    void connectTCPAdb();
    void connectDevice();

signals:
    void stageChanged(quint16 stage);

protected slots:
    void deviceConnected(bool success,
                         const QString &serial,
                         const QString &deviceName,
                         const QSize &size);
    void deviceDisconnected(const QString &serial);

private:
    quint16 m_stage;
    QString m_uuid;
    QString m_ip;
    QString m_tcpSerial;
    uint16_t m_tcpPort = 5555;

    qsc::AdbProcess *m_adb;

    QQmlApplicationEngine engine;
    QrCodeProxy *m_qrCodeProxy;
    DeviceProxy *m_deviceProxy;
    VideoFrameProvider *m_videoFrameProvider;

    qsc::IDevice *m_device;

    bool m_isMaximized;
    bool m_isFullScreen;

    QStringList getIPs();
    void updateQrCode();
    void setStage(STAGE stage);

    void handleAdbProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);
    void listDevices();
};

#endif // ANDROID_ANDROIDMAINWINDOW_H
