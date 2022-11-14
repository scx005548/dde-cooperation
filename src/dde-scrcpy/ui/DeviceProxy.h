#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>

namespace qsc {
class IDevice;
}

class QMouseEvent;

class DeviceProxy : public QObject {
    Q_OBJECT

public:
    explicit DeviceProxy(QObject *parent = nullptr);

    void setDevice(qsc::IDevice *device);

    Q_INVOKABLE void onMouseMove(qreal x,
                                 qreal y,
                                 int button,
                                 int buttons,
                                 int modifiers,
                                 const QSizeF &frameSize,
                                 const QSizeF &showSize);
    Q_INVOKABLE void onPressed(qreal x,
                                 qreal y,
                                 int button,
                                 int buttons,
                                 int modifiers,
                                 const QSizeF &frameSize,
                                 const QSizeF &showSize);
    Q_INVOKABLE void onReleased(qreal x,
                                 qreal y,
                                 int button,
                                 int buttons,
                                 int modifiers,
                                 const QSizeF &frameSize,
                                 const QSizeF &showSize);
    Q_INVOKABLE void onBackButtonClicked();
    Q_INVOKABLE void onHomeButtonClicked();
    Q_INVOKABLE void onOverviewButtonClicked();
    Q_INVOKABLE void onSwitchScreenButtonClicked();

signals:

private:
    qsc::IDevice *m_device;
    bool m_screenClosed;
};

#endif // BACKEND_H
