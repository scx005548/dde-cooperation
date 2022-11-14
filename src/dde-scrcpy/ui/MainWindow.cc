#include "MainWindow.h"

#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDebug>

#include <QQuickWidget>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>

#include "QtScrcpyCore.h"
#include "DeviceProxy.h"
#include "VideoFrameProvider.h"

DTK_USE_NAMESPACE

using namespace qsc;

#define DDE_PROTO_VER 13

static const QString DDE_COOPERATION_DBUS_SERVICE = "com.deepin.Cooperation";
static const QString DDE_COOPERATION_DBUS_PATH = "/com/deepin/Cooperation";
static const QString DDE_COOPERATION_DBUS_INTERFACE = "com.deepin.Cooperation";

MainWindow::MainWindow(const QString &ip, QWidget *parent)
    : DMainWindow(parent)
    , m_ip(ip)
    , m_adb(new AdbProcess(this))
    , m_view(new QQuickWidget(this))
    , m_deviceProxy(new DeviceProxy(this))
    , m_videoFrameProvider(new VideoFrameProvider(this)) {

    m_view->engine()->rootContext()->setContextProperty("device", m_deviceProxy);
    m_view->engine()->rootContext()->setContextProperty("videoFrameProvider", m_videoFrameProvider);
    m_view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_view->setSource(QUrl("qrc:/ui/VideoPlayer.qml"));
    setCentralWidget(m_view);

    connect(m_adb, &AdbProcess::adbProcessResult, this, &MainWindow::handleAdbProcessResult);

    // openTCPAdb();
    m_tcpSerial = QString("%1:%2").arg(m_ip).arg(m_tcpPort);
    connectDevice();
}

MainWindow::~MainWindow() {
}

void MainWindow::handleAdbProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult) {
    QStringList args = m_adb->arguments();
    switch (processResult) {
    case AdbProcess::AER_SUCCESS_START:
        break;
    case AdbProcess::AER_ERROR_START:
        break;
    case AdbProcess::AER_SUCCESS_EXEC:
        if (args.contains("tcpip")) {
            connectTCPAdb();
        } else if (args.contains("connect")) {
            // m_wifiSerial = args[args.indexOf("connect") + 1];
            //                if (!m_jacksuRuned) {
            //                    qInfo() << "startRemoteJacksu:" << serial;
            //                    delayMs(300);
            //                    startRemoteJacksu();
            //                }
            QTimer::singleShot(500, [this]() {
                connectDevice(); // 打开投屏端口准备投屏
            });
            break;
        }
        break;
    case AdbProcess::AER_ERROR_EXEC:
        break;
    case AdbProcess::AER_ERROR_MISSING_BINARY:
        break;
    };
}

void MainWindow::listDevices() {
    m_adb->execute("", QStringList{"devices", "-l"});
}

void MainWindow::openTCPAdb() {
    m_adb->execute("", QStringList{"tcpip", QString::number(m_tcpPort)});
}

void MainWindow::connectTCPAdb() {
    m_tcpSerial = QString("%1:%2").arg(m_ip).arg(m_tcpPort);
    m_adb->execute("", QStringList{"connect", m_tcpSerial});
}

void MainWindow::connectDevice() {
    DeviceParams params;
    params.serial = m_tcpSerial;
    m_device = IDevice::create(params, this);
    m_deviceProxy->setDevice(m_device);

    connect(m_device, &IDevice::deviceConnected, this, &MainWindow::deviceConnected);
    connect(m_device, &IDevice::deviceDisconnected, this, &MainWindow::deviceDisconnected);

    m_device->connectDevice();
}

void MainWindow::deviceConnected(bool success,
                                 const QString &serial,
                                 const QString &deviceName,
                                 const QSize &size) {
    if (!success) {
        return;
    }

    m_device->registerDeviceObserver(m_videoFrameProvider);

    setWindowTitle(deviceName + " - " + serial);

    show();
}

void MainWindow::deviceDisconnected([[maybe_unused]] const QString &serial) {
    close();
}
