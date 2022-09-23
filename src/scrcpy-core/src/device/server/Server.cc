#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>

#include "Server.h"

#define DEVICE_NAME_FIELD_LENGTH 64
#define MAX_CONNECT_COUNT 30
#define MAX_RESTART_COUNT 1

Server::Server(QObject *parent)
    : QObject(parent) {
    connect(&m_serverSocket, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *tmp = m_serverSocket.nextPendingConnection();
        if (dynamic_cast<VideoSocket *>(tmp)) {
            m_videoSocket = dynamic_cast<VideoSocket *>(tmp);
            if (!m_videoSocket->isValid() || !readInfo(m_videoSocket, m_deviceName, m_deviceSize)) {
                stop();
                emit serverStarted(false);
            }
        } else {
            m_controlSocket = tmp;
            if (m_controlSocket && m_controlSocket->isValid()) {
                // we don't need the server socket anymore
                // just m_videoSocket is ok
                m_serverSocket.close();
                emit serverStarted(true, m_deviceName, m_deviceSize);
            } else {
                stop();
                emit serverStarted(false);
            }
            stopAcceptTimeoutTimer();
        }
    });
}

Server::~Server() {
}

bool Server::start() {
    // At the application level, the device part is "the server" because it
    // serves video stream and control. However, at the network level, the
    // client listens and the server connects to the client. That way, the
    // client can listen before starting the server app, so there is no need to
    // try to connect until the server socket is listening on the device.
    m_serverSocket.setMaxPendingConnections(2);
    if (!m_serverSocket.listen()) {
        qCritical() << "Could not listen";
        return false;
    }

    m_port = m_serverSocket.serverPort();

    return true;
}

bool Server::connectTo() {
    if (!m_videoSocket) {
        startAcceptTimeoutTimer();
        return true;
    }

    return true;
}

uint16_t Server::getPort() {
    return m_port;
}

void Server::timerEvent(QTimerEvent *event) {
    if (event && m_acceptTimeoutTimer == event->timerId()) {
        stopAcceptTimeoutTimer();
        emit serverStarted(false);
    }
}

VideoSocket *Server::removeVideoSocket() {
    VideoSocket *socket = m_videoSocket;
    m_videoSocket = Q_NULLPTR;
    return socket;
}

QTcpSocket *Server::getControlSocket() {
    return m_controlSocket;
}

void Server::stop() {
    stopAcceptTimeoutTimer();

    if (m_controlSocket) {
        m_controlSocket->close();
        m_controlSocket->deleteLater();
    }
    // ignore failure
    m_serverSocket.close();
}

bool Server::readInfo(VideoSocket *videoSocket, QString &deviceName, QSize &size) {
    unsigned char buf[DEVICE_NAME_FIELD_LENGTH + 4];
    if (videoSocket->bytesAvailable() <= (DEVICE_NAME_FIELD_LENGTH + 4)) {
        videoSocket->waitForReadyRead(300);
    }

    qint64 len = videoSocket->read((char *)buf, sizeof(buf));
    if (len < DEVICE_NAME_FIELD_LENGTH + 4) {
        qInfo("Could not retrieve device information");
        return false;
    }
    buf[DEVICE_NAME_FIELD_LENGTH - 1] = '\0'; // in case the client sends garbage
    // strcpy is safe here, since name contains at least DEVICE_NAME_FIELD_LENGTH bytes
    // and strlen(buf) < DEVICE_NAME_FIELD_LENGTH
    deviceName = (char *)buf;
    size.setWidth((buf[DEVICE_NAME_FIELD_LENGTH] << 8) | buf[DEVICE_NAME_FIELD_LENGTH + 1]);
    size.setHeight((buf[DEVICE_NAME_FIELD_LENGTH + 2] << 8) | buf[DEVICE_NAME_FIELD_LENGTH + 3]);
    return true;
}

void Server::startAcceptTimeoutTimer() {
    stopAcceptTimeoutTimer();
    m_acceptTimeoutTimer = startTimer(1000);
}

void Server::stopAcceptTimeoutTimer() {
    if (m_acceptTimeoutTimer) {
        killTimer(m_acceptTimeoutTimer);
        m_acceptTimeoutTimer = 0;
    }
}
