#ifndef WRAPPERS_INPUTEMITTORWRAPPER_H
#define WRAPPERS_INPUTEMITTORWRAPPER_H

#include <filesystem>

#include <QObject>
#include <QProcess>

#include "common.h"

class QLocalServer;
class QLocalSocket;
class QProcess;

class Manager;
class Machine;

class InputEmittorWrapper : public QObject {
    Q_OBJECT

public:
    explicit InputEmittorWrapper(InputDeviceType type);
    ~InputEmittorWrapper();
    void setMachine(const std::weak_ptr<Machine> &machine);
    bool emitEvent(unsigned int type, unsigned int code, int value) noexcept;

private slots:
    void onProcessClosed(int exitCode, QProcess::ExitStatus exitStatus);
    void handleNewConnection();
    void onReceived();
    void onDisconnected();

private:
    QLocalServer *m_server;
    QLocalSocket *m_conn;
    QProcess *m_process;

    InputDeviceType m_type;
};

#endif // !WRAPPERS_INPUTEMITTORWRAPPER_H
