#include "confirmdialog.h"

#include <QApplication>
#include <QLabel>
#include <QFile>
#include <QDebug>

const char Accept = 0x01;
const char Reject = 0x00;

ConfirmDialog::ConfirmDialog(const QString &ip, const QString &machineName, int pipeFd)
    : DDialog()
    , m_titleLabel(new QLabel(this))
    , m_contentLabel(new QLabel(this))
{
    setAttribute(Qt::WA_QuitOnClose);
    setFocus(Qt::MouseFocusReason);

    QFont font = m_titleLabel->font();
    font.setBold(true);
    font.setPixelSize(16);
    m_titleLabel->setFont(font);
    m_titleLabel->setText(tr("Cooperation request confirm:"));
    addContent(m_titleLabel, Qt::AlignTop | Qt::AlignHCenter);

    QString content = QString(tr("Machine(%1) %2 request cooperation")).arg(ip).arg(machineName);
    m_contentLabel->setText(content);
    addContent(m_contentLabel, Qt::AlignBottom | Qt::AlignHCenter);

    QStringList btnTexts;
    btnTexts << tr("reject") << tr("accept");
    addButtons(btnTexts);

    connect(this, &ConfirmDialog::buttonClicked, this, [pipeFd](int index, const QString &text) {
        Q_UNUSED(text);
        QFile writePipe;
        if (!writePipe.open(pipeFd, QIODevice::WriteOnly)) {
            qWarning() << "Cooperation open pipeFd has error!";
            return;
        }

        char data = index == 1 ? Accept : Reject;
        writePipe.write(&data, 1);
        writePipe.close();
    });

    connect(this, &ConfirmDialog::closed, this, [](){
        qApp->quit();
    });

}
