#ifndef VIDEOFORM_H
#define VIDEOFORM_H

#include <QPointer>
#include <QWidget>

#include "QtScrcpyCore.h"

class KeepRatioWidget;
class ToolForm;
class FileHandler;
class QYUVOpenGLWidget;
class QLabel;
class QVideoWidget;
class QVBoxLayout;
class VideoForm : public QWidget, public qsc::DeviceObserver {
    Q_OBJECT
public:
    explicit VideoForm(QWidget *parent = nullptr);
    ~VideoForm();

    void setDevice(qsc::IDevice *device);
    void staysOnTop(bool top = true);
    void updateShowSize(const QSize &newSize);
    void setSerial(const QString &serial);
    QRect getGrabCursorRect();
    const QSize &frameSize();
    void resizeSquare();
    void removeBlackRect();
    void showFPS(bool show);
    void switchFullScreen();

    bool isHost();

private:
    void onFrame(const QVideoFrame &frame) override;
    void updateFPS(quint32 fps) override;
    void grabCursor(bool grab) override;

    QMargins getMargins(bool vertical);
    void initUI();

    void showToolForm(bool show = true);
    void moveCenter();
    void installShortcut();
    QRect getScreenRect();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // ui
    QVBoxLayout *m_layout;
    KeepRatioWidget *m_keepRatioWidget;
    QVideoWidget *m_videoWidget;
    QPointer<ToolForm> m_toolForm;
    QPointer<QWidget> m_loadingWidget;
    QPointer<QLabel> m_fpsLabel;

    // inside member
    QSize m_frameSize;
    QSize m_normalSize;
    QPoint m_dragPosition;
    float m_widthHeightRatio = 0.5f;
    QPoint m_fullScreenBeforePos;
    QString m_serial;
    qsc::IDevice *m_device;
};

#endif // VIDEOFORM_H
