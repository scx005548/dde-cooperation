#include "MachineDBusAdaptor.h"

#include "uvxx/Async.h"

#include "Manager.h"
#include "Machine.h"

MachineDBusAdaptor::MachineDBusAdaptor(Manager *manager,
                                       Machine *machine,
                                       QDBusConnection bus,
                                       const std::shared_ptr<uvxx::Loop> &uvLoop)
    : QDBusAbstractAdaptor(machine)
    , m_manager(manager)
    , m_machine(machine)
    , m_bus(bus)
    , m_uvLoop(uvLoop)
    , m_async(std::make_shared<uvxx::Async>(m_uvLoop)) {
}

QString MachineDBusAdaptor::getUUID() const {
    return m_uuid;
}

QString MachineDBusAdaptor::getName() const {
    return m_name;
}

QString MachineDBusAdaptor::getIP() const {
    return m_ip;
}

quint16 MachineDBusAdaptor::getPort() const {
    return m_port;
}

quint32 MachineDBusAdaptor::getOS() const {
    return m_os;
}

quint32 MachineDBusAdaptor::getCompositor() const {
    return m_compositor;
}

bool MachineDBusAdaptor::getConnected() const {
    return m_connected;
}

bool MachineDBusAdaptor::getDeviceSharing() const {
    return m_deviceSharing;
}

quint16 MachineDBusAdaptor::getDirection() const {
    return m_direction;
}

bool MachineDBusAdaptor::getSharedClipboard() const {
    return m_sharedClipboard;
}

void MachineDBusAdaptor::Connect(const QDBusMessage &message) const {
    if (m_machine->connected()) {
        return;
    }

    if ((m_machine->isPcMachine() && m_manager->hasPcMachinePaired()) ||
        (m_machine->isAndroid() && m_manager->hasAndroidPaired())) {
        // TODO tips
        m_bus.send(message.createErrorReply(
            {QDBusError::Failed,
             QStringLiteral("This machine is cooperating with another machine!")}));
        return;
    }

    // m_async
    m_async->wake(uvxx::memFunc(m_machine, &Machine::connect));
}

void MachineDBusAdaptor::Disconnect() const {
    if (!m_machine->connected()) {
        return;
    }

    m_machine->disconnect();
}

void MachineDBusAdaptor::RequestDeviceSharing(const QDBusMessage &message) const {
    if (m_machine->m_deviceSharing) {
        m_bus.send(message.createErrorReply(
            {QDBusError::Failed, QStringLiteral("already sharing devices")}));
        return;
    }

    if (!m_machine->connected()) {
        m_bus.send(
            message.createErrorReply({QDBusError::AccessDenied, QStringLiteral("connect first")}));
        return;
    }

    m_async->wake(uvxx::memFunc(m_machine, &Machine::requestDeviceSharing));
}

void MachineDBusAdaptor::StopDeviceSharing(const QDBusMessage &message) const {
    if (!m_machine->m_deviceSharing) {
        m_bus.send(message.createErrorReply({QDBusError::Failed, QStringLiteral("")}));
        return;
    }

    m_async->wake(uvxx::memFunc(m_machine, &Machine::stopDeviceSharing));
}

void MachineDBusAdaptor::SetFlowDirection(quint16 direction, const QDBusMessage &message) const {
    if (direction > FLOW_DIRECTION_LEFT) {
        m_bus.send(
            message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("invalid args")}));
        return;
    }

    m_async->wake([this, direction = static_cast<FlowDirection>(direction)]() {
        m_machine->setFlowDirection(direction);
    });
}

void MachineDBusAdaptor::SendFiles(const QStringList &paths, const QDBusMessage &message) const {
    if (paths.empty()) {
        m_bus.send(
            message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("paths is empty")}));
        return;
    }

    m_async->wake([this, paths]() { m_machine->sendFiles(paths); });
}

void MachineDBusAdaptor::updateUUID(const QString &uuid) {
    m_uuid = uuid;
    emit uuidChanged(uuid);
}

void MachineDBusAdaptor::updateName(const QString &name) {
    m_name = name;
    emit nameChanged(name);
}

void MachineDBusAdaptor::updateIP(const QString &ip) {
    m_ip = ip;
    emit ipChanged(ip);
}

void MachineDBusAdaptor::updatePort(quint16 port) {
    m_port = port;
    emit portChanged(port);
}

void MachineDBusAdaptor::updateOS(quint32 os) {
    m_os = os;
    emit osChanged(os);
}

void MachineDBusAdaptor::updateCompositor(quint32 compositor) {
    m_compositor = compositor;
    emit compositorChanged(compositor);
}

void MachineDBusAdaptor::updateConnected(bool v) {
    m_connected = v;
    emit connectedChanged(v);
}

void MachineDBusAdaptor::updateDeviceSharing(bool v) {
    m_deviceSharing = v;
    emit deviceSharingChanged(v);
}

void MachineDBusAdaptor::updateDirection(quint16 v) {
    m_direction = v;
    emit directionChanged(v);
}

void MachineDBusAdaptor::updateSharedClipboard(bool v) {
    m_sharedClipboard = v;
    emit sharedClipboardChanged(v);
}
