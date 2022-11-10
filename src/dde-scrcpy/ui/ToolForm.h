#ifndef TOOLFORM_H
#define TOOLFORM_H

#include <QPointer>
#include <QWidget>

#include "QtScrcpyCore.h"

class Device;
class ToolForm : public QWidget {
    Q_OBJECT

public:
    explicit ToolForm(qsc::IDevice *device, QWidget *parent);
    ~ToolForm();

    void setSerial(const QString &serial);
    bool isHost();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void on_backBtn_clicked();
    void on_overviewBtn_clicked();
    void on_homeBtn_clicked();
    void on_switchScreenBtn_clicked();

private:
    QPoint m_dragPosition;
    QString m_serial;
    qsc::IDevice *m_device;
    bool m_screenClosed = false;
    bool m_showTouch = false;
    bool m_isHost = false;
};

#endif // TOOLFORM_H
