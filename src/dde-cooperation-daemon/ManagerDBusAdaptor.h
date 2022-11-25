#ifndef MANAGERDBUSADAPTOR_H
#define MANAGERDBUSADAPTOR_H

#include <QtDBus>
#include <QVector>

class Manager;

class ManagerDBusAdaptor : public QDBusAbstractAdaptor {
    friend class Manager;

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Cooperation1")

    Q_PROPERTY(QVector<QDBusObjectPath> Machines READ getMachines NOTIFY machinesChanged)
    Q_PROPERTY(
        QStringList CooperatedMachines READ getCooperatedMachines NOTIFY cooperatedMachinesChanged)
    Q_PROPERTY(bool DeviceSharingSwitch READ getDeviceSharingSwitch WRITE setDeviceSharingSwitch)
    Q_PROPERTY(QString FilesStoragePath READ getFilesStoragePath)
    Q_PROPERTY(bool SharedClipboard READ getSharedClipboard)
    Q_PROPERTY(bool SharedDevices READ getSharedDevices)

public:
    ManagerDBusAdaptor(Manager *manager, QDBusConnection bus);

public: // D-Bus properties
    QVector<QDBusObjectPath> getMachines() const;
    QStringList getCooperatedMachines() const;
    bool getDeviceSharingSwitch() const;
    void setDeviceSharingSwitch(bool value) const;
    QString getFilesStoragePath() const;
    bool getSharedClipboard() const;
    bool getSharedDevices() const;

public slots: // D-Bus methods
    QString GetUUID(const QDBusMessage &message) const;
    void Scan(const QDBusMessage &message) const;
    void Knock(const QString &ip, quint16 port) const;
    void SendFile(const QStringList &files, int osType, const QDBusMessage &message) const;
    void SetFilesStoragePath(const QString &path, const QDBusMessage &message) const;
    void OpenSharedClipboard(bool on) const;
    void OpenSharedDevices(bool on) const;

signals:
    void machinesChanged(QVector<QDBusObjectPath>);
    void cooperatedMachinesChanged(QStringList);
    void deviceSharingSwitchChanged(bool);
    void fileStoragePathChanged(QString v);
    void sharedClipboardChanged(bool v);
    void sharedDevicesChanged(bool v);

protected: // update properties
    void updateMachines(const QVector<QDBusObjectPath> &machines);
    void updateCooperatedMachines(const QStringList &UUIDs);
    void updateDeviceSharingSwitch(bool v);
    void updateFileStoragePath(QString v);
    void updateSharedClipboard(bool v);
    void updateSharedDevices(bool v);

private:
    Manager *m_manager;
    QDBusConnection m_bus;

    QVector<QDBusObjectPath> m_machines;
    QStringList m_cooperatedMachines;
    bool m_deviceSharingSwitch;
    QString m_fileStoragePath;
    bool m_sharedClipboard;
    bool m_sharedDevices;
};

#endif // !MANAGERDBUSADAPTOR_H
