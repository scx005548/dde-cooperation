#ifndef DDE_COOPERATION_RECONNECTDIALOG_H
#define DDE_COOPERATION_RECONNECTDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class ReconnectDialog : public DDialog {
    Q_OBJECT

public:
    explicit ReconnectDialog(const QString &machineName);

signals:
    void onOperated(bool tryAgain);
};

#endif // DDE_COOPERATION_RECONNECTDIALOG_H
