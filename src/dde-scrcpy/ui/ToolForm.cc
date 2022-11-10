#include "VideoForm.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QToolButton>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include "ToolForm.h"

ToolForm::ToolForm(qsc::IDevice *device, QWidget *parent)
    : QWidget(parent)
    , m_device(device) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    QToolButton *backBtn = new QToolButton(this);
    backBtn->setIcon(QIcon(":/icons/back.svg"));
    connect(backBtn, &QToolButton::clicked, this, &ToolForm::on_backBtn_clicked);
    layout->addWidget(backBtn);

    QToolButton *overviewBtn = new QToolButton(this);
    overviewBtn->setIcon(QIcon(":/icons/overview.svg"));
    connect(overviewBtn, &QToolButton::clicked, this, &ToolForm::on_overviewBtn_clicked);
    layout->addWidget(overviewBtn);

    QToolButton *homeBtn = new QToolButton(this);
    homeBtn->setIcon(QIcon(":/icons/home.svg"));
    connect(homeBtn, &QToolButton::clicked, this, &ToolForm::on_homeBtn_clicked);
    layout->addWidget(homeBtn);

    QToolButton *switchScreenBtn = new QToolButton(this);
    switchScreenBtn->setIcon(QIcon(":/icons/close_screen.svg"));
    connect(switchScreenBtn, &QToolButton::clicked, this, &ToolForm::on_switchScreenBtn_clicked);
    layout->addWidget(switchScreenBtn);
}

ToolForm::~ToolForm() {
}

void ToolForm::setSerial(const QString &serial) {
    m_serial = serial;
}

bool ToolForm::isHost() {
    return m_isHost;
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

void ToolForm::on_backBtn_clicked() {
    m_device->postGoBack();
}

void ToolForm::on_overviewBtn_clicked() {
    m_device->postAppSwitch();
}

void ToolForm::on_homeBtn_clicked() {
    m_device->postGoHome();
}

void ToolForm::on_switchScreenBtn_clicked() {
    m_screenClosed = !m_screenClosed;
    m_device->setScreenPowerMode(m_screenClosed);
}
