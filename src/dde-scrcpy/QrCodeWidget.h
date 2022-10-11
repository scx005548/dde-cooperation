#ifndef QRCODEWIDGET_H
#define QRCODEWIDGET_H

#include <QWidget>

class QrCodeWidget : public QWidget {
    Q_OBJECT
public:
    QrCodeWidget(QWidget *parent = nullptr);

    void setText(const QString &text);

    virtual void paintEvent(QPaintEvent *event) override;

private:
    QString m_text;
};

#endif // !QRCODEWIDGET_H
