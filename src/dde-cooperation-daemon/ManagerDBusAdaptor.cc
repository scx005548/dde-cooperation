#include "ManagerDBusAdaptor.h"

#include <filesystem>

#include "uvxx/Async.h"

#include "config.h"
#include "Manager.h"

namespace fs = std::filesystem;

static bool isSubDir(fs::path p, fs::path root) {
    qWarning() << "root:" << QString::fromStdString(root.string());
    for (;;) {
        qWarning() << "p:" << QString::fromStdString(p.string());
        if (p == root) {
            return true;
        }
        if (!p.has_parent_path()) {
            return false;
        }
        p = p.parent_path();
        if (p == "/") {
            return false;
        }
    }

    return false;
}

ManagerDBusAdaptor::ManagerDBusAdaptor(Manager *manager, QDBusConnection bus)
    : QDBusAbstractAdaptor(manager)
    , m_manager(manager)
    , m_bus(bus)
    , m_deviceSharingSwitch(false)
    , m_fileStoragePath(false)
    , m_sharedClipboard(false)
    , m_sharedDevices(false) {
}

QVector<QDBusObjectPath> ManagerDBusAdaptor::getMachines() const {
    return m_machines;
}

QStringList ManagerDBusAdaptor::getCooperatedMachines() const {
    return m_cooperatedMachines;
}

bool ManagerDBusAdaptor::getDeviceSharingSwitch() const {
    return m_deviceSharingSwitch;
}

void ManagerDBusAdaptor::setDeviceSharingSwitch(bool value) const {
    m_manager->setDeviceSharingSwitch(value);
}

QString ManagerDBusAdaptor::getFilesStoragePath() const {
    return m_fileStoragePath;
}

bool ManagerDBusAdaptor::getSharedClipboard() const {
    return m_sharedClipboard;
}

bool ManagerDBusAdaptor::getSharedDevices() const {
    return m_sharedDevices;
}

QString ManagerDBusAdaptor::GetUUID(const QDBusMessage &message) const {
    QString sender = message.service();
    auto reply = m_bus.interface()->servicePid(sender);
    if (!reply.isValid()) {
        qWarning() << "GetConnectionUnixProcessID:" << reply.error();
        m_bus.send(
            message.createErrorReply({QDBusError::InternalError, QStringLiteral("Iternal Error")}));
        return QString();
    }

    uint pid = reply.value();
    QString callerPath = QFile::symLinkTarget(QString("/proc/%1/exe").arg(pid));
    if (!isSubDir(callerPath.toStdString(), EXECUTABLE_INSTALL_DIR)) {
        qWarning() << "GetUUID callerPath:" << callerPath << "Access Denied";
        m_bus.send(
            message.createErrorReply({QDBusError::AccessDenied, QStringLiteral("Access Denied")}));
        return QString();
    }

    return QString::fromStdString(m_manager->uuid());
}

void ManagerDBusAdaptor::Scan(const QDBusMessage &message) const {
    if (m_manager->m_deviceSharingSwitch) {
        message.createErrorReply({QDBusError::Failed, "DeviceSharing Switch close!"});
        qWarning() << "DeviceSharing Switch close";
        return;
    }

    m_manager->m_async->wake([this]() { m_manager->scan(); });
}

void ManagerDBusAdaptor::Knock(const QString &ip, quint16 port) const {
    m_manager->m_async->wake([this, ip = ip.toStdString(), port]() { m_manager->ping(ip, port); });
}

void ManagerDBusAdaptor::SendFile(const QStringList &files,
                                  int osType,
                                  const QDBusMessage &message) const {
    if (files.empty()) {
        message.createErrorReply({QDBusError::InvalidArgs, "filepath param has error!"});
        return;
    }

    if (!m_manager->sendFile(files, osType)) {
        message.createErrorReply({QDBusError::Failed, "Target machine not found!"});
        return;
    }
}

void ManagerDBusAdaptor::SetFilesStoragePath(const QString &path,
                                             const QDBusMessage &message) const {
    if (path.isEmpty()) {
        message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("empty path")});
        return;
    }

    if (!QDir(path).exists()) {
        message.createErrorReply({QDBusError::Failed, QStringLiteral("path is not a directory")});
        return;
    }

    m_manager->setFileStoragePath(path);
}

void ManagerDBusAdaptor::OpenSharedClipboard(bool on) const {
    m_manager->openSharedClipboard(on);
}

void ManagerDBusAdaptor::OpenSharedDevices(bool on) const {
    m_manager->openSharedDevices(on);
}

void ManagerDBusAdaptor::updateMachines(const QVector<QDBusObjectPath> &machines) {
    m_machines = machines;
    emit machinesChanged(m_machines);
}

void ManagerDBusAdaptor::updateCooperatedMachines(const QStringList &UUIDs) {
    m_cooperatedMachines = UUIDs;
    emit cooperatedMachinesChanged(UUIDs);
}

void ManagerDBusAdaptor::updateDeviceSharingSwitch(bool v) {
    m_deviceSharingSwitch = v;
    emit deviceSharingSwitchChanged(v);
}

void ManagerDBusAdaptor::updateFileStoragePath(QString v) {
    m_fileStoragePath = v;
    emit fileStoragePathChanged(v);
}

void ManagerDBusAdaptor::updateSharedClipboard(bool v) {
    m_sharedClipboard = v;
    emit sharedClipboardChanged(v);
}

void ManagerDBusAdaptor::updateSharedDevices(bool v) {
    m_sharedDevices = v;
    emit sharedDevicesChanged(v);
}
