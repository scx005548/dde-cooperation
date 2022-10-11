#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QrCodeWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setText(const QString &text);

private:
    QrCodeWidget *m_qrcode;
};

#endif // MAINWINDOW_H
