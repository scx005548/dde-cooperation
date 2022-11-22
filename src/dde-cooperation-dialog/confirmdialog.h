#ifndef DDE_COOPERATION_CONFIRMDIALOG_H
#define DDE_COOPERATION_CONFIRMDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class QLabel;

class ConfirmDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ConfirmDialog(const QString &ip, const QString &machineName, int pipeFd);

private:
    void writeBackResult(int pipeFd, bool isAccepted);

private:
    QLabel *m_titleLabel;
    QLabel *m_contentLabel;
};

#endif // DDE_COOPERATION_CONFIRMDIALOG_H
