#include "AndroidMachineDBusAdaptor.h"

#include "AndroidMachine.h"

AndroidMachineDBusAdaptor::AndroidMachineDBusAdaptor(AndroidMachine *machine,
                                                     QDBusConnection bus,
                                                     const QString &path)
    : QObject(machine)
    , m_machine(machine)
    , m_path(path)
    , m_bus(bus) {
    m_bus.registerObject(m_path,
                         this,
                         QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

AndroidMachineDBusAdaptor::~AndroidMachineDBusAdaptor() {
    m_bus.unregisterObject(m_path);
}

void AndroidMachineDBusAdaptor::StartCast(const QDBusMessage &message) {
    if (!m_machine->connected()) {
        auto reply = message.createErrorReply(QDBusError::Failed, "connect first");
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    m_machine->startCast();
}
