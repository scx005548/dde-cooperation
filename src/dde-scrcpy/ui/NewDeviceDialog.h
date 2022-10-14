#ifndef NEWDEVICEDIALOG_H
#define NEWDEVICEDIALOG_H

#include <DMainWindow>
#include <QLabel>

class QrCodeWidget;

class NewDeviceDialog : public DTK_WIDGET_NAMESPACE::DMainWindow {
    Q_OBJECT

public:
    NewDeviceDialog(const QString &machineId, int protoVer, QWidget *parent = nullptr);

private:
    QLabel *m_titleLabel;
    QLabel *m_descLabel;
    QrCodeWidget *m_qrcode;

    QStringList getIPs();
};

#endif // !NEWDEVICEDIALOG_H
