#include "MainWindow.h"

#include <QLayout>

#include "QrCodeWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setFixedSize(300, 300);

    m_qrcode = new QrCodeWidget(this);
    m_qrcode->adjustSize();
    setCentralWidget(m_qrcode);
}

MainWindow::~MainWindow() {
}

void MainWindow::setText(const QString &text) {
    m_qrcode->setText(text);
}
