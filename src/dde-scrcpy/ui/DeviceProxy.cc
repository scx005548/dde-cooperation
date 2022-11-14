#include "DeviceProxy.h"

#include "QtScrcpyCore.h"

DeviceProxy::DeviceProxy(QObject *parent)
    : QObject(parent)
    , m_screenClosed(false) {
}

void DeviceProxy::setDevice(qsc::IDevice *device) {
    m_device = device;
}

void DeviceProxy::onMouseMove(qreal x,
                              qreal y,
                              int button,
                              int buttons,
                              int modifiers,
                              const QSizeF &frameSize,
                              const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseMove,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onPressed(qreal x,
                            qreal y,
                            int button,
                            int buttons,
                            int modifiers,
                            const QSizeF &frameSize,
                            const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseButtonPress,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onReleased(qreal x,
                             qreal y,
                             int button,
                             int buttons,
                             int modifiers,
                             const QSizeF &frameSize,
                             const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseButtonRelease,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onBackButtonClicked() {
    m_device->postGoBack();
}

void DeviceProxy::onHomeButtonClicked() {
    m_device->postGoHome();
}

void DeviceProxy::onOverviewButtonClicked() {
    m_device->postAppSwitch();
}

void DeviceProxy::onSwitchScreenButtonClicked() {
    m_screenClosed = !m_screenClosed;
    m_device->setScreenPowerMode(m_screenClosed);
}
