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

    Q_INVOKABLE void onKeyPressed(int key,
                                  int modifiers,
                                  const QString &text,
                                  bool autorep,
                                  ushort count);
    Q_INVOKABLE void onKeyReleased(int key,
                                   int modifiers,
                                   const QString &text,
                                   bool autorep,
                                   ushort count);
    Q_INVOKABLE void onMouseMove(qreal x,
                                 qreal y,
                                 int button,
                                 int buttons,
                                 int modifiers,
                                 const QSizeF &frameSize,
                                 const QSizeF &showSize);
    Q_INVOKABLE void onMouseButtonPressed(qreal x,
                                          qreal y,
                                          int button,
                                          int buttons,
                                          int modifiers,
                                          const QSizeF &frameSize,
                                          const QSizeF &showSize);
    Q_INVOKABLE void onMouseButtonReleased(qreal x,
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
    bool m_screenOpen;
};

#endif // BACKEND_H
