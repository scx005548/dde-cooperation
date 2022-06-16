#include "Backend.h"

#include "MachineModel.h"

static const QString cooperationService = QStringLiteral("com.deepin.Cooperation");
static const QString cooperationPath = QStringLiteral("/com/deepin/Cooperation");
static const QString cooperationInterface = QStringLiteral("com.deepin.Cooperation");

static const QString machineInterface = QStringLiteral("com.deepin.Cooperation.Machine");

Backend::Backend(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::systemBus())
    , m_cooperation(cooperationService, cooperationPath, cooperationInterface, m_bus)
    , m_machinePaths(m_cooperation.property("Machines").value<QList<QDBusObjectPath>>())
    , m_machineModel(new MachineModel(this)) {
    m_bus.connect(cooperationService,
                  cooperationPath,
                  cooperationInterface,
                  "org.freedesktop.DBus.Properties",
                  "PropertiesChanged",
                  this,
                  SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));
    for (auto &path : m_machinePaths) {
        addMachineByObjectPath(path);
    }
}

QObject *Backend::getMachineModel() {
    return m_machineModel;
}

void Backend::onPropertiesChanged(QString interface,
                                  QVariantMap map,
                                  [[maybe_unused]] QStringList list) {
    if (interface != cooperationInterface) {
        return;
    }

    if (map.contains("Machines")) {
        // TODO: 优化
        m_machineModel->clear();
        m_machinePaths = map["Machines"].value<QList<QDBusObjectPath>>();
        for (auto &path : m_machinePaths) {
            addMachineByObjectPath(path);
        }
    }
}

void Backend::addMachineByObjectPath(const QDBusObjectPath &path) {
    QDBusInterface m(cooperationService, path.path(), machineInterface, m_bus);
    auto name = m.property("Name").toString();
    auto os = m.property("OS").toString();

    m_machineModel->addMachine(Machine{name, os});
}
