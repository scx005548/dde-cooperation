// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include "global_defines.h"

#include <QDialog>
#include <QLabel>
#include <QCloseEvent>

namespace daemon_cooperation {

class ConfirmDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfirmDialog(QWidget *parent = nullptr);
    ~ConfirmDialog() override;

    void initUI();

    void setAction(const QStringList &actions);
    void setContent(const QString &content);
    void setSavePath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override
    {
        event->ignore();
        this->hide();
    }

private:
    QLabel *contentLabel { nullptr };
    QPushButton *confirmButton { nullptr };
    QPushButton *cancelButton { nullptr };
    QPushButton *viewButton { nullptr };
    QString savePath;
};

}   // namespace daemon_cooperation

#endif   // CONFIRMDIALOG_H
