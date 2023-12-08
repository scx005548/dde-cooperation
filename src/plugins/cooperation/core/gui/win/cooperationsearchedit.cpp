﻿// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationsearchedit.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

using namespace cooperation_core;
const char *iconpath = ":/icons/deepin/builtin/texts/search_icon.svg";

CooperationSearchEdit::CooperationSearchEdit(QWidget *parent) : QFrame(parent)
{
    setStyleSheet("border-radius: 8px;"
                  "background-color: rgba(0,0,0,0.1);");

    setFixedSize(460, 35);
    setFocusPolicy(Qt::StrongFocus);

    searchIcon = new QLabel(this);
    searchIcon->setPixmap(QIcon(iconpath).pixmap(10, 10));
    searchIcon->setGeometry(178, 8, 20, 20);
    searchIcon->setStyleSheet("background-color: none;");

    searchEdit = new QLineEdit(this);
    searchEdit->setFixedHeight(35);
    searchEdit->setPlaceholderText(tr("                    Search"));
    searchEdit->setStyleSheet("background-color: rgba(0,0,0,0);");

    QFont font;
    font.setPointSize(11);
    searchEdit->setFont(font);
    searchEdit->installEventFilter(this);
    connect(searchEdit, &QLineEdit::textChanged, this, &CooperationSearchEdit::textChanged);

    closeBtn = new QToolButton(this);
    closeBtn->setIcon(QIcon(":/icons/deepin/builtin/icons/tab_close_normal.svg"));
    closeBtn->setIconSize(QSize(35, 35));
    closeBtn->setStyleSheet("background-color: rgba(0,0,0,0);");
    closeBtn->setGeometry(420, 0, 35, 35);
    closeBtn->setVisible(false);
    connect(searchEdit, &QLineEdit::textChanged, this,
            [this](const QString &str) { closeBtn->setVisible(!str.isEmpty()); });
    connect(closeBtn, &QToolButton::clicked, this, [this] { searchEdit->setText(""); });

    QHBoxLayout *mainLayout = new QHBoxLayout();
    setLayout(mainLayout);
    mainLayout->addSpacing(40);
    mainLayout->addWidget(searchEdit);
    mainLayout->setMargin(0);
    mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
}

QString CooperationSearchEdit::text() const
{
    return searchEdit->text();
}

void CooperationSearchEdit::setPlaceholderText(const QString &text)
{
    //TODO: windows
}

bool CooperationSearchEdit::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == searchEdit) {
        QIcon searchicon(iconpath);
        if (event->type() == QEvent::FocusIn) {
            searchIcon->setPixmap(searchicon.pixmap(18, 18));
            searchIcon->setGeometry(15, 8, 20, 20);
            searchEdit->setPlaceholderText("");
        } else if (event->type() == QEvent::FocusOut && searchEdit->text().isEmpty()) {
            searchIcon->setPixmap(searchicon.pixmap(10, 10));
            searchIcon->setGeometry(178, 8, 20, 20);
            searchEdit->setPlaceholderText(tr("                    Search"));
        }
    }
    return QObject::eventFilter(obj, event);
}

void CooperationSearchEdit::focusInEvent(QFocusEvent *event)
{
    searchEdit->setFocus();
    QWidget::focusInEvent(event);
}
