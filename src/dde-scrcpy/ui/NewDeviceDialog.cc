#include "NewDeviceDialog.h"

#include <QVBoxLayout>
#include <QUrlQuery>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>

#include "uibase/QrCodeWidget.h"

NewDeviceDialog::NewDeviceDialog(const QString &machineId, int protoVer, QWidget *parent)
    : DMainWindow(parent) {
    QWidget *window = new QWidget();
    setCentralWidget(window);

    QVBoxLayout *layout = new QVBoxLayout(this);
    window->setLayout(layout);

    m_titleLabel = new QLabel(tr("使用“统信UOS助手”App扫码投屏"), this);
    layout->addWidget(m_titleLabel);

    m_descLabel = new QLabel(tr("首次连接需要通过USB连接手机与电脑"), this);
    layout->addWidget(m_descLabel);

    QUrlQuery query;
    query.addQueryItem("dde", QString::number(protoVer));
    query.addQueryItem("host", machineId);
    query.addQueryItem("ip", getIPs().join(";"));
    QString qrcodeText = query.toString();
    qDebug() << "QR code info:" << qrcodeText;

    m_qrcode = new QrCodeWidget(this);
    m_qrcode->setMinimumSize(100, 100);
    m_qrcode->setText(qrcodeText);
    layout->addWidget(m_qrcode);
}

QStringList NewDeviceDialog::getIPs() {
    QStringList ips;
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.isLoopback()) {
            continue;
        }

        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            ips.append(address.toString());
        }
    }

    return ips;
}
