#ifndef MACHINE_MACHINEDBUSADAPTOR_H
#define MACHINE_MACHINEDBUSADAPTOR_H

#include <QtDBus>
#include <QVector>

class Manager;
class Machine;

namespace uvxx {
class Loop;
class Async;
} // namespace uvxx

class MachineDBusAdaptor : public QDBusAbstractAdaptor {
    friend class Machine;

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Cooperation1.Machine")

    Q_PROPERTY(QString UUID READ getUUID NOTIFY uuidChanged)
    Q_PROPERTY(QString Name READ getName NOTIFY nameChanged)
    Q_PROPERTY(QString IP READ getIP NOTIFY ipChanged)
    Q_PROPERTY(quint16 Port READ getPort NOTIFY portChanged)
    Q_PROPERTY(quint32 OS READ getOS NOTIFY osChanged)
    Q_PROPERTY(quint32 Compositor READ getCompositor NOTIFY compositorChanged)
    Q_PROPERTY(bool Connected READ getConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool DeviceSharing READ getDeviceSharing NOTIFY deviceSharingChanged)
    Q_PROPERTY(quint16 Direction READ getDirection NOTIFY directionChanged)
    Q_PROPERTY(bool SharedClipboard READ getSharedClipboard NOTIFY sharedClipboardChanged)

public:
    MachineDBusAdaptor(Manager *manager,
                       Machine *machine,
                       QDBusConnection bus,
                       const std::shared_ptr<uvxx::Loop> &uvLoop);

public: // D-Bus properties
    QString getUUID() const;
    QString getName() const;
    QString getIP() const;
    quint16 getPort() const;
    quint32 getOS() const;
    quint32 getCompositor() const;
    bool getConnected() const;
    bool getDeviceSharing() const;
    quint16 getDirection() const;
    bool getSharedClipboard() const;

public slots: // D-Bus methods
    void Connect(const QDBusMessage &message) const;
    void Disconnect() const;
    void RequestDeviceSharing(const QDBusMessage &message) const;
    void StopDeviceSharing(const QDBusMessage &message) const;
    void SetFlowDirection(quint16 direction, const QDBusMessage &message) const;
    void SendFiles(const QStringList &paths, const QDBusMessage &message) const;

signals:
    void uuidChanged(const QString &);
    void nameChanged(const QString &);
    void ipChanged(const QString &);
    void portChanged(quint16);
    void osChanged(quint32);
    void compositorChanged(quint32);
    void connectedChanged(bool v);
    void deviceSharingChanged(bool v);
    void directionChanged(quint16 v);
    void sharedClipboardChanged(bool v);

protected: // update properties
    void updateUUID(const QString &uuid);
    void updateName(const QString &name);
    void updateIP(const QString &ip);
    void updatePort(quint16);
    void updateOS(quint32);
    void updateCompositor(quint32);
    void updateConnected(bool v);
    void updateDeviceSharing(bool v);
    void updateDirection(quint16 v);
    void updateSharedClipboard(bool v);

private:
    Manager *m_manager;
    Machine *m_machine;
    QDBusConnection m_bus;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Async> m_async;
};

#endif // !MACHINE_MACHINEDBUSADAPTOR_H
