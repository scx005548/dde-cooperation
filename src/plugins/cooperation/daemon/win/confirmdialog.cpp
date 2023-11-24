// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "confirmdialog.h"
#include "global_defines.h"

#include "common/constant.h"
#include <utils/cooperationutil.h>
#include <config/configmanager.h>

#include <QDesktopServices>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

using namespace daemon_cooperation;

ConfirmDialog::ConfirmDialog(QWidget *parent)
    : QDialog(parent)
{
    initUI();
}

ConfirmDialog::~ConfirmDialog()
{
}

void ConfirmDialog::initUI()
{
    setWindowTitle(tr("File transfer"));
    setFixedSize(380, 234);

    QVBoxLayout *layout = new QVBoxLayout(this);
    contentLabel = new QLabel();
    layout->addWidget(contentLabel);
    layout->setAlignment(Qt::AlignCenter);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    confirmButton = new QPushButton(tr("accept"));
    buttonLayout->addWidget(confirmButton);

    cancelButton = new QPushButton(tr("reject"));
    buttonLayout->addWidget(cancelButton);

    viewButton = new QPushButton(tr("view"));
    buttonLayout->addWidget(viewButton);

    layout->addLayout(buttonLayout);

    connect(confirmButton, &QPushButton::clicked, this, [this] {
        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_CONFIRM);
        this->hide();
    });

    connect(cancelButton, &QPushButton::clicked, this, [this] {
        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_REFUSED);
        this->hide();
    });

    connect(viewButton, &QPushButton::clicked, this, [this] {
        if (savePath.isEmpty()) {
            auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
            savePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
        this->hide();
    });
    setLayout(layout);
}

void ConfirmDialog::setAction(const QStringList &actions)
{
    confirmButton->setVisible(actions.contains("accept"));
    cancelButton->setVisible(actions.contains("reject"));
    viewButton->setVisible(actions.contains("view"));
}

void ConfirmDialog::setContent(const QString &content)
{
    contentLabel->setText(content);
}

void ConfirmDialog::setSavePath(const QString &path)
{
    savePath = path;
}
