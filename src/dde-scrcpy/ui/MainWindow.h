#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <QStackedWidget>

#include "AdbProcess.h"

#include "uibase/KeepRatioWidget.h"
#include "ui/VideoForm.h"

namespace qsc {
class IDevice;
}

class MainWindow : public DTK_WIDGET_NAMESPACE::DMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString &ip, QWidget *parent = nullptr);
    ~MainWindow();

protected slots:
    void deviceConnected(bool success,
                         const QString &serial,
                         const QString &deviceName,
                         const QSize &size);
    void deviceDisconnected(const QString &serial);

private:
    QString m_ip;
    QString m_tcpSerial;
    uint16_t m_tcpPort = 5555;

    qsc::AdbProcess *m_adb;

    QStackedWidget *m_stackedWidget;
    VideoForm *m_videoForm;

    qsc::IDevice *m_device;

    void handleAdbProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);
    void listDevices();
    void openTCPAdb();
    void connectTCPAdb();
    void connectDevice();
};

#endif // MAINWINDOW_H
