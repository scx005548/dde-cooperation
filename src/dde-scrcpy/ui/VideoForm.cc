#include "VideoForm.h"

#include <QGuiApplication>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QWindow>
#include <QtWidgets/QHBoxLayout>
#include <QDebug>
#include <QVideoFrame>
#include <QVideoWidget>
#include <QVideoSurfaceFormat>
#include <QAbstractVideoSurface>

#include "ToolForm.h"
#include "uibase/KeepRatioWidget.h"
#include "util/mousetap/MouseTap.h"

VideoForm::VideoForm(QWidget *parent)
    : QWidget(parent)
    , m_keepRatioWidget(new KeepRatioWidget(this)) {
    initUI();
    installShortcut();
    updateShowSize(size());
}

VideoForm::~VideoForm() {
}

void VideoForm::initUI() {
    m_layout = new QVBoxLayout(this);
    setLayout(m_layout);
    m_layout->addWidget(m_keepRatioWidget);

    m_videoWidget = new QVideoWidget(this);
    m_keepRatioWidget->setWidget(m_videoWidget);
    m_keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

    m_fpsLabel = new QLabel(m_videoWidget);
    QFont ft;
    ft.setPointSize(15);
    ft.setWeight(QFont::Light);
    ft.setBold(true);
    m_fpsLabel->setFont(ft);
    m_fpsLabel->move(5, 15);
    m_fpsLabel->setMinimumWidth(100);
    m_fpsLabel->setStyleSheet(R"(QLabel {color: #00FF00;})");

    setMouseTracking(true);
    m_videoWidget->setMouseTracking(true);
    m_keepRatioWidget->setMouseTracking(true);
}

void VideoForm::setDevice(qsc::IDevice *device) {
    m_device = device;
}

QRect VideoForm::getGrabCursorRect() {
    QRect rc;
#if defined(Q_OS_WIN32)
    rc = QRect(m_keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_OSX)
    rc = m_videoWidget->geometry();
    rc.setTopLeft(m_keepRatioWidget->mapToGlobal(rc.topLeft()));
    rc.setBottomRight(m_keepRatioWidget->mapToGlobal(rc.bottomRight()));

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#elif defined(Q_OS_LINUX)
    rc = QRect(m_keepRatioWidget->mapToGlobal(m_videoWidget->pos()), m_videoWidget->size());
    // high dpi support -- taken from the WIN32 section and untested
    rc.setTopLeft(rc.topLeft() * m_videoWidget->devicePixelRatioF());
    rc.setBottomRight(rc.bottomRight() * m_videoWidget->devicePixelRatioF());

    rc.setX(rc.x() + 10);
    rc.setY(rc.y() + 10);
    rc.setWidth(rc.width() - 20);
    rc.setHeight(rc.height() - 20);
#endif
    return rc;
}

const QSize &VideoForm::frameSize() {
    return m_frameSize;
}

void VideoForm::resizeSquare() {
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    resize(screenRect.height(), screenRect.height());
}

void VideoForm::removeBlackRect() {
    resize(m_keepRatioWidget->goodSize());
}

void VideoForm::showFPS(bool show) {
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setVisible(show);
}

void VideoForm::setSerial(const QString &serial) {
    m_serial = serial;
}

void VideoForm::showToolForm(bool show) {
    if (!m_toolForm) {
        m_toolForm = new ToolForm(m_device, this);
        m_toolForm->setSerial(m_serial);
    }
    // m_toolForm->move(pos().x() + geometry().width(), pos().y() + 30);
    m_toolForm->setVisible(show);
    m_layout->addWidget(m_toolForm);
}

void VideoForm::moveCenter() {
    QRect screenRect = getScreenRect();
    if (screenRect.isEmpty()) {
        qWarning() << "getScreenRect is empty";
        return;
    }
    // 窗口居中
    move(screenRect.center() - QRect(0, 0, size().width(), size().height()).center());
}

void VideoForm::installShortcut() {
    QShortcut *shortcut = nullptr;

    // switchFullScreen
    shortcut = new QShortcut(QKeySequence("Ctrl+f"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { switchFullScreen(); });

    // resizeSquare
    shortcut = new QShortcut(QKeySequence("Ctrl+g"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { resizeSquare(); });

    // removeBlackRect
    shortcut = new QShortcut(QKeySequence("Ctrl+w"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { removeBlackRect(); });

    // postGoHome
    shortcut = new QShortcut(QKeySequence("Ctrl+h"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { m_device->postGoHome(); });

    // postGoBack
    shortcut = new QShortcut(QKeySequence("Ctrl+b"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { m_device->postGoBack(); });

    // postAppSwitch
    shortcut = new QShortcut(QKeySequence("Ctrl+s"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postAppSwitch(); });

    // postGoMenu
    shortcut = new QShortcut(QKeySequence("Ctrl+m"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { m_device->postGoMenu(); });

    // postVolumeUp
    shortcut = new QShortcut(QKeySequence("Ctrl+up"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postVolumeUp(); });

    // postVolumeDown
    shortcut = new QShortcut(QKeySequence("Ctrl+down"), this);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postVolumeDown(); });

    // postPower
    shortcut = new QShortcut(QKeySequence("Ctrl+p"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postPower(); });

    shortcut = new QShortcut(QKeySequence("Ctrl+o"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        emit m_device->setScreenPowerMode(false);
    });

    // expandNotificationPanel
    shortcut = new QShortcut(QKeySequence("Ctrl+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        emit m_device->expandNotificationPanel();
    });

    // collapsePanel
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+n"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->collapsePanel(); });

    // copy
    shortcut = new QShortcut(QKeySequence("Ctrl+c"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postCopy(); });

    // cut
    shortcut = new QShortcut(QKeySequence("Ctrl+x"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->postCut(); });

    // clipboardPaste
    shortcut = new QShortcut(QKeySequence("Ctrl+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        emit m_device->setDeviceClipboard();
    });

    // setDeviceClipboard
    shortcut = new QShortcut(QKeySequence("Ctrl+Shift+v"), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, this, [this]() { emit m_device->clipboardPaste(); });
}

QRect VideoForm::getScreenRect() {
    QRect screenRect;
    QWidget *win = window();
    if (!win) {
        return screenRect;
    }

    QWindow *winHandle = win->windowHandle();
    QScreen *screen = QGuiApplication::primaryScreen();
    if (winHandle) {
        screen = winHandle->screen();
    }
    if (!screen) {
        return screenRect;
    }

    screenRect = screen->availableGeometry();
    return screenRect;
}

QMargins VideoForm::getMargins(bool vertical) {
    QMargins margins;
    if (vertical) {
        margins = QMargins(10, 68, 12, 62);
    } else {
        margins = QMargins(68, 12, 62, 10);
    }
    return margins;
}

void VideoForm::updateShowSize(const QSize &newSize) {
    if (m_frameSize != newSize) {
        m_frameSize = newSize;

        m_widthHeightRatio = 1.0f * newSize.width() / newSize.height();
        m_keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);

        bool vertical = m_widthHeightRatio < 1.0f ? true : false;
        QSize showSize = newSize;
        QRect screenRect = getScreenRect();
        if (screenRect.isEmpty()) {
            qWarning() << "getScreenRect is empty";
            return;
        }
        if (vertical) {
            showSize.setHeight(qMin(newSize.height(), screenRect.height() - 200));
            showSize.setWidth(showSize.height() * m_widthHeightRatio);
        } else {
            showSize.setWidth(qMin(newSize.width(), screenRect.width() / 2));
            showSize.setHeight(showSize.width() / m_widthHeightRatio);
        }

        if (isFullScreen()) {
            switchFullScreen();
        }

        if (isMaximized()) {
            showNormal();
        }

        if (showSize != size()) {
            resize(showSize);
            moveCenter();
        }
    }
}

void VideoForm::switchFullScreen() {
    if (isFullScreen()) {
        // 横屏全屏铺满全屏，恢复时，恢复保持宽高比
        if (m_widthHeightRatio > 1.0f) {
            m_keepRatioWidget->setWidthHeightRatio(m_widthHeightRatio);
        }

        showNormal();
        // back to normal size.
        resize(m_normalSize);
        // fullscreen window will move (0,0). qt bug?
        move(m_fullScreenBeforePos);

#ifdef Q_OS_OSX
        // setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        // show();
#endif
        showToolForm(true);
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS);
#endif
    } else {
        // 横屏全屏铺满全屏，不保持宽高比
        if (m_widthHeightRatio > 1.0f) {
            m_keepRatioWidget->setWidthHeightRatio(-1.0f);
        }

        // record current size before fullscreen, it will be used to rollback size after exit
        // fullscreen.
        m_normalSize = size();

        m_fullScreenBeforePos = pos();
        // 这种临时增加标题栏再全屏的方案会导致收不到mousemove事件，导致setmousetrack失效
        // mac fullscreen must show title bar
#ifdef Q_OS_OSX
        // setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
#endif
        showToolForm(false);
        showFullScreen();

        // 全屏状态禁止电脑休眠、息屏
#ifdef Q_OS_WIN32
        ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
    }
}

bool VideoForm::isHost() {
    return m_toolForm->isHost();
}

void VideoForm::updateFPS(quint32 fps) {
    // qDebug() << "FPS:" << fps;
    if (!m_fpsLabel) {
        return;
    }
    m_fpsLabel->setText(QString("FPS:%1").arg(fps));
}

void VideoForm::grabCursor(bool grab) {
    QRect rc = getGrabCursorRect();
    MouseTap::getInstance()->enableMouseEventTap(rc, grab);
}

void VideoForm::onFrame(const QVideoFrame &frame) {
    QVideoSurfaceFormat format(frame.size(), QVideoFrame::Format_ARGB32);
    m_videoWidget->videoSurface()->start(format);

    m_videoWidget->videoSurface()->present(frame.image().convertToFormat(QImage::Format_ARGB32));
    m_videoWidget->show();
}

void VideoForm::staysOnTop(bool top) {
    bool needShow = false;
    if (isVisible()) {
        needShow = true;
    }
    setWindowFlag(Qt::WindowStaysOnTopHint, top);
    if (m_toolForm) {
        m_toolForm->setWindowFlag(Qt::WindowStaysOnTopHint, top);
    }
    if (needShow) {
        show();
    }
}

void VideoForm::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        if (m_device && !m_device->isCurrentCustomKeymap()) {
            m_device->postGoHome();
            return;
        }
    }

    if (event->button() == Qt::RightButton) {
        if (m_device && !m_device->isCurrentCustomKeymap()) {
            m_device->postGoBack();
            return;
        }
    }

    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!m_device) {
            return;
        }
        event->setLocalPos(m_videoWidget->mapFrom(this, event->localPos().toPoint()));
        emit m_device->mouseEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());

        // debug keymap pos
        if (event->button() == Qt::LeftButton) {
            qreal x = event->localPos().x() / m_videoWidget->size().width();
            qreal y = event->localPos().y() / m_videoWidget->size().height();
            QString posTip = QString(R"("pos": {"x": %1, "y": %2})").arg(x).arg(y);
            qInfo() << posTip.toStdString().c_str();
        }
    } else {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
}

void VideoForm::mouseReleaseEvent(QMouseEvent *event) {
    if (m_dragPosition.isNull()) {
        event->setLocalPos(m_videoWidget->mapFrom(this, event->localPos().toPoint()));
        // local check
        QPointF local = event->localPos();
        if (local.x() < 0) {
            local.setX(0);
        }
        if (local.x() > m_videoWidget->width()) {
            local.setX(m_videoWidget->width());
        }
        if (local.y() < 0) {
            local.setY(0);
        }
        if (local.y() > m_videoWidget->height()) {
            local.setY(m_videoWidget->height());
        }
        event->setLocalPos(local);
        emit m_device->mouseEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
    } else {
        m_dragPosition = QPoint(0, 0);
    }
}

void VideoForm::mouseMoveEvent(QMouseEvent *event) {
    if (m_videoWidget->geometry().contains(event->pos())) {
        event->setLocalPos(m_videoWidget->mapFrom(this, event->localPos().toPoint()));
        emit m_device->mouseEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
    } else if (!m_dragPosition.isNull()) {
        if (event->buttons() & Qt::LeftButton) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
}

void VideoForm::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !m_videoWidget->geometry().contains(event->pos())) {
        if (!isMaximized()) {
            removeBlackRect();
        }
    }

    if (event->button() == Qt::RightButton && !m_device->isCurrentCustomKeymap()) {
        emit m_device->postBackOrScreenOn(event->type() == QEvent::MouseButtonPress);
    }

    if (m_videoWidget->geometry().contains(event->pos())) {
        event->setLocalPos(m_videoWidget->mapFrom(this, event->localPos().toPoint()));
        emit m_device->mouseEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::wheelEvent(QWheelEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (m_videoWidget->geometry().contains(event->position().toPoint())) {
        QPointF pos = m_videoWidget->mapFrom(this, event->position().toPoint());
        QWheelEvent wheelEvent(pos,
                               event->globalPosition(),
                               event->pixelDelta(),
                               event->angleDelta(),
                               event->buttons(),
                               event->modifiers(),
                               event->phase(),
                               event->inverted());
#else
    if (m_videoWidget->geometry().contains(event->pos())) {
        if (!device) {
            return;
        }
        QPointF pos = m_videoWidget->mapFrom(this, event->pos());

        QWheelEvent wheelEvent(pos,
                               event->globalPosF(),
                               event->pixelDelta(),
                               event->angleDelta(),
                               event->delta(),
                               event->orientation(),
                               event->buttons(),
                               event->modifiers(),
                               event->phase(),
                               event->source(),
                               event->inverted());
#endif
        emit m_device->wheelEvent(&wheelEvent, m_videoWidget->frameSize(), m_videoWidget->size());
    }
}

void VideoForm::keyPressEvent(QKeyEvent *event) {
    if (Qt::Key_Escape == event->key() && !event->isAutoRepeat() && isFullScreen()) {
        switchFullScreen();
    }

    emit m_device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::keyReleaseEvent(QKeyEvent *event) {
    emit m_device->keyEvent(event, m_videoWidget->frameSize(), m_videoWidget->size());
}

void VideoForm::paintEvent(QPaintEvent *paint) {
    Q_UNUSED(paint)
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void VideoForm::showEvent(QShowEvent *event) {
    Q_UNUSED(event)
    if (!isFullScreen()) {
        showToolForm();
    }
}

void VideoForm::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    QSize goodSize = m_keepRatioWidget->goodSize();
    if (goodSize.isEmpty()) {
        return;
    }
    QSize curSize = size();
    // 限制VideoForm尺寸不能小于keepRatioWidget good size
    if (m_widthHeightRatio > 1.0f) {
        // hor
        if (curSize.height() <= goodSize.height()) {
            setMinimumHeight(goodSize.height());
        } else {
            setMinimumHeight(0);
        }
    } else {
        // ver
        if (curSize.width() <= goodSize.width()) {
            setMinimumWidth(goodSize.width());
        } else {
            setMinimumWidth(0);
        }
    }
}

void VideoForm::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event)
    m_device->disconnectDevice();
}

void VideoForm::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();
}

void VideoForm::dragMoveEvent(QDragMoveEvent *event) {
    Q_UNUSED(event)
}

void VideoForm::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event)
}

void VideoForm::dropEvent(QDropEvent *event) {
    const QMimeData *qm = event->mimeData();
    QList<QUrl> urls = qm->urls();

    for (const QUrl &url : urls) {
        QString file = url.toLocalFile();
        QFileInfo fileInfo(file);

        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "QtScrcpy", tr("file does not exist"), QMessageBox::Ok);
            continue;
        }

        if (fileInfo.isFile() && fileInfo.suffix() == "apk") {
            emit m_device->installApkRequest(file);
            continue;
        }
        emit m_device->pushFileRequest(file, QStringLiteral("/sdcard/") + fileInfo.fileName());
    }
}
