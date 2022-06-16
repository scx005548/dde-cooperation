#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QList>
#include <QPoint>
#include <QDBusConnection>
#include <QDBusInterface>

class MachineModel;

class Backend : public QObject {
    Q_OBJECT

public:
    explicit Backend(QObject *parent = nullptr);

    QObject *getMachineModel();

signals:
    void machinesChanged();

private slots:
    void onPropertiesChanged(QString interface, QVariantMap map, QStringList list);

private:
    QDBusConnection m_bus;
    QDBusInterface m_cooperation;

    QList<QDBusObjectPath> m_machinePaths;
    MachineModel *m_machineModel;

    void handleMachinesChanged();
    void addMachineByObjectPath(const QDBusObjectPath &path);
};

#endif // BACKEND_H
