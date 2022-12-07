#ifndef DDE_COOPERATION_DAEMON_CONFIRMDIALOG_H
#define DDE_COOPERATION_DAEMON_CONFIRMDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class QLabel;

class ConfirmDialog : public DDialog {
    Q_OBJECT

public:
    explicit ConfirmDialog(const QString &ip, const QString &machineName);

signals:
    void onConfirmed(bool accepted);

private:
    QLabel *m_titleLabel;
    QLabel *m_contentLabel;
};

#endif // DDE_COOPERATION_DAEMON_CONFIRMDIALOG_H
