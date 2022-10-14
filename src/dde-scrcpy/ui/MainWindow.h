#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <QStackedWidget>

#include "AdbProcess.h"

#include "ui/VideoForm.h"

class MainWindow : public DTK_WIDGET_NAMESPACE::DMainWindow {
    Q_OBJECT

public:
    MainWindow(const QString &ip, QWidget *parent = nullptr);
    ~MainWindow();

private:
    QString m_ip;
    QString m_tcpSerial;
    uint16_t m_tcpPort = 5555;

    qsc::AdbProcess *m_adb;

    QStackedWidget *m_stackedWidget;
    VideoForm *m_videoForm;

    void handleAdbProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult);
    void listDevices();
    void openTCPAdb();
    void connectTCPAdb();
    void connectDevice();
};

#endif // MAINWINDOW_H
