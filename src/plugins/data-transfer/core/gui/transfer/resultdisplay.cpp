﻿#include "resultdisplay.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QApplication>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QStackedWidget>
#include <utils/transferhepler.h>

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget() {}

void ResultDisplayWidget::initUI()
{
    setStyleSheet(".ResultDisplayWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    iconLabel->setAlignment(Qt::AlignCenter);

    titileLabel = new QLabel(tr("Transfer completed"), this);
    titileLabel->setFont(StyleHelper::font(1));
    titileLabel->setAlignment(Qt::AlignCenter);

    tiptextlabel = new QLabel(this);
    tiptextlabel->setFont(StyleHelper::font(3));
    tiptextlabel->setText(QString(tr("Partial information migration failed, please go to UOS for manual transfer")));
    tiptextlabel->setAlignment(Qt::AlignCenter);
    tiptextlabel->setVisible(false);

    resultWindow = new ResultWindow();

    QHBoxLayout *textBrowerlayout = new QHBoxLayout();
    textBrowerlayout->setAlignment(Qt::AlignCenter);
    textBrowerlayout->addWidget(resultWindow);

    ButtonLayout *buttonLayout = new ButtonLayout();
    QPushButton *backButton = buttonLayout->getButton1();
    backButton->setText(tr("Back"));
    QPushButton *nextButton = buttonLayout->getButton2();
    nextButton->setText(tr("Exit"));

    connect(backButton, &QPushButton::clicked, this, &ResultDisplayWidget::nextPage);
    connect(nextButton, &QPushButton::clicked, qApp, &QApplication::quit);

    mainLayout->addSpacing(40);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(tiptextlabel);
    mainLayout->addLayout(textBrowerlayout);
    mainLayout->addLayout(buttonLayout);
    addResult("QString name", true, "QString reason");
    connect(TransferHelper::instance(), &TransferHelper::addResult, this,
            &ResultDisplayWidget::addResult);
#ifdef linux
    connect(TransferHelper::instance(), &TransferHelper::transferFinished, this, [this] {
        TransferHelper::instance()->sendMessage("add_result", processText);
    });
#endif
}

void ResultDisplayWidget::nextPage()
{
    TransferHelper::instance()->sendMessage("change_page", "startTransfer");
    QTimer::singleShot(1000, this, [this] {
        QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
        if (stackedWidget) {
            if (stackedWidget->currentIndex() == PageName::resultwidget)
                stackedWidget->setCurrentIndex(PageName::choosewidget);
        } else {
            WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                    "nullptr";
        }
    });
}

void ResultDisplayWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        titileLabel->setStyleSheet("QLabel{color:rgb(0,26,46);}");
        tiptextlabel->setStyleSheet("QLabel{color:rgb(0,26,46);}");
    } else {
        // dark
        setStyleSheet(".ResultDisplayWidget{-color: rgb(37, 37, 37); border-radius: 10px;}");
        titileLabel->setStyleSheet("QLabel{color:rgb(192,198,212);}");
        tiptextlabel->setStyleSheet("QLabel{color:rgb(192,198,212);}");
    }
    resultWindow->changeTheme(theme);
}

void ResultDisplayWidget::addResult(QString name, bool success, QString reason)
{
    QString info, color;
    if (!success) {
        color = "#FF5736";
        setStatus(false);
    } else
        color = "#6199CA";
    name = ellipsizedText(name, 430, QFont());

    resultWindow->updateContent(name, reason, success);
    QString res = success ? "true" : "false";
    processText.append(name + " " + res + " " + reason + ";");
}

void ResultDisplayWidget::clear()
{
    resultWindow->clear();
    processText.clear();
    setStatus(true);
}

void ResultDisplayWidget::setStatus(bool success)
{
    tiptextlabel->setVisible(!success);
    if (success) {
        titileLabel->setText(tr("Transfer completed"));
        iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    } else {
        titileLabel->setText(tr("Transfer completed partially"));
        iconLabel->setPixmap(QIcon(":/icon/success half-96.svg").pixmap(96, 96));
    }
}

QString ResultDisplayWidget::ellipsizedText(const QString &input, int maxLength, const QFont &font)
{
    QFontMetrics fontMetrics(font);
    int textWidth = fontMetrics.horizontalAdvance(input);   // 获取文本的宽度

    if (textWidth <= maxLength) {
        return input;   // 如果文本宽度未超出限制，则直接返回原文本
    } else {
        QString ellipsizedString = fontMetrics.elidedText(input, Qt::ElideMiddle, maxLength);   // 使用省略号替代超出部分
        return ellipsizedString;
    }
}

ResultWindow::ResultWindow(QFrame *parent)
    : ProcessDetailsWindow(parent)
{
    init();
}

ResultWindow::~ResultWindow()
{
}

void ResultWindow::updateContent(const QString &name, const QString &type, bool success)
{
    int maxWith = 400;
    QString nameT = QFontMetrics(StyleHelper::font(3)).elidedText(name, Qt::ElideRight, maxWith);
    QString typeT = QFontMetrics(StyleHelper::font(3)).elidedText(type, Qt::ElideRight, maxWith);

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());

    for (int col = 0; col < model->columnCount(); ++col) {
        QModelIndex index = model->index(0, col);
        QString itemName = model->data(index, Qt::DisplayRole).toString();
        if (itemName == nameT) {
            model->setData(index, typeT, Qt::ToolTipRole);
            return;
        }
    }

    QStandardItem *item = new QStandardItem();
    item->setData(nameT, Qt::DisplayRole);
    item->setData(typeT, Qt::ToolTipRole);
    if (success) {
        item->setData(0, Qt::StatusTipRole);
    } else {
        item->setData(1, Qt::StatusTipRole);
    }

    model->appendRow(item);
}

void ResultWindow::changeTheme(int theme)
{
    if (theme == 1) {
        setStyleSheet(".ResultWindow{background-color: rgba(0, 0, 0, 0.08);"
                      "border-radius: 10px;"
                      "padding: 10px 30px 10px 10px;"
                      "}");
    } else {
        // dark
        setStyleSheet(".ResultWindow{background-color: rgba(255,255,255, 0.08);"
                      "border-radius: 10px;"
                      "padding: 10px 30px 10px 10px;"
                      "}");
    }

    ProcessWindowItemDelegate *delegate = qobject_cast<ProcessWindowItemDelegate *>(this->itemDelegate());
    delegate->setTheme(theme);
}

void ResultWindow::init()
{
    setStyleSheet(".ResultWindow{background-color: rgba(0, 0, 0, 0.08);"
                  "border-radius: 10px;"
                  "padding: 10px 30px 10px 10px;"
                  "}");
    QStandardItemModel *model = new QStandardItemModel(this);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setModel(model);
    ProcessWindowItemDelegate *delegate = new ProcessWindowItemDelegate();
    delegate->setStageColor(QColor(Qt::red));
    setItemDelegate(delegate);
    setFixedSize(460,112);
}
