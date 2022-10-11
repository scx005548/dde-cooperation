#include "VideoForm.h"

#include <QDebug>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include "ToolForm.h"
#include "ui_ToolForm.h"

ToolForm::ToolForm(qsc::IDevice *device, QWidget *adsorbWidget, AdsorbPositions adsorbPos)
    : MagneticWidget(adsorbWidget, adsorbPos)
    , ui(new Ui::ToolForm)
    , m_device(device) {
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    // setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    initStyle();
}

ToolForm::~ToolForm() {
    delete ui;
}

void ToolForm::setSerial(const QString &serial) {
    m_serial = serial;
}

bool ToolForm::isHost() {
    return m_isHost;
}

void ToolForm::initStyle() {
    // IconHelper::Instance()->SetIcon(ui->fullScreenBtn, QChar(0xf0b2), 15);
    // IconHelper::Instance()->SetIcon(ui->menuBtn, QChar(0xf096), 15);
    // IconHelper::Instance()->SetIcon(ui->homeBtn, QChar(0xf1db), 15);
    // //IconHelper::Instance()->SetIcon(ui->returnBtn, QChar(0xf104), 15);
    // IconHelper::Instance()->SetIcon(ui->returnBtn, QChar(0xf053), 15);
    // IconHelper::Instance()->SetIcon(ui->appSwitchBtn, QChar(0xf24d), 15);
    // IconHelper::Instance()->SetIcon(ui->volumeUpBtn, QChar(0xf028), 15);
    // IconHelper::Instance()->SetIcon(ui->volumeDownBtn, QChar(0xf027), 15);
    // IconHelper::Instance()->SetIcon(ui->openScreenBtn, QChar(0xf06e), 15);
    // IconHelper::Instance()->SetIcon(ui->closeScreenBtn, QChar(0xf070), 15);
    // IconHelper::Instance()->SetIcon(ui->powerBtn, QChar(0xf011), 15);
    // IconHelper::Instance()->SetIcon(ui->expandNotifyBtn, QChar(0xf103), 15);
    // IconHelper::Instance()->SetIcon(ui->screenShotBtn, QChar(0xf0c4), 15);
    // IconHelper::Instance()->SetIcon(ui->touchBtn, QChar(0xf111), 15);
}

void ToolForm::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void ToolForm::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event)
}

void ToolForm::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void ToolForm::showEvent(QShowEvent *event) {
    Q_UNUSED(event)
    qDebug() << "show event";
}

void ToolForm::hideEvent(QHideEvent *event) {
    Q_UNUSED(event)
    qDebug() << "hide event";
}

void ToolForm::on_fullScreenBtn_clicked() {
    dynamic_cast<VideoForm *>(parent())->switchFullScreen();
}

void ToolForm::on_returnBtn_clicked() {
    m_device->postGoBack();
}

void ToolForm::on_homeBtn_clicked() {
    m_device->postGoHome();
}

void ToolForm::on_menuBtn_clicked() {
    m_device->postGoMenu();
}

void ToolForm::on_appSwitchBtn_clicked() {
    m_device->postAppSwitch();
}

void ToolForm::on_powerBtn_clicked() {
    m_device->postPower();
}

void ToolForm::on_screenShotBtn_clicked() {
    m_device->screenshot();
}

void ToolForm::on_volumeUpBtn_clicked() {
    m_device->postVolumeUp();
}

void ToolForm::on_volumeDownBtn_clicked() {
    m_device->postVolumeDown();
}

void ToolForm::on_closeScreenBtn_clicked() {
    m_device->setScreenPowerMode(false);
}

void ToolForm::on_expandNotifyBtn_clicked() {
    m_device->expandNotificationPanel();
}

void ToolForm::on_touchBtn_clicked() {
    m_showTouch = !m_showTouch;
    m_device->showTouch(m_showTouch);
}

void ToolForm::on_openScreenBtn_clicked() {
    m_device->setScreenPowerMode(true);
}
