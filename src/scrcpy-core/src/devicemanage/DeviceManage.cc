#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include "DeviceManage.h"
#include "Device.h"
#include "Stream.h"

namespace qsc {

#define DM_MAX_DEVICES_NUM 1000

IDeviceManage &IDeviceManage::getInstance() {
    static DeviceManage dm;
    return dm;
}

DeviceManage::DeviceManage() {
    Stream::init();
}

DeviceManage::~DeviceManage() {
    Stream::deInit();
}

QPointer<IDevice> DeviceManage::getDevice(const QString &serial) {
    if (!m_devices.contains(serial)) {
        return QPointer<IDevice>();
    }
    return m_devices[serial];
}

bool DeviceManage::connectDevice(qsc::DeviceParams params) {
    if (params.serial.trimmed().isEmpty()) {
        return false;
    }
    if (m_devices.contains(params.serial)) {
        return false;
    }
    if (DM_MAX_DEVICES_NUM < m_devices.size()) {
        qInfo("over the maximum number of connections");
        return false;
    }
    IDevice *device = new Device(params);
    connect(device, &Device::deviceConnected, this, &DeviceManage::onDeviceConnected);
    connect(device, &Device::deviceDisconnected, this, &DeviceManage::onDeviceDisconnected);
    if (!device->connectDevice()) {
        delete device;
        return false;
    }
    m_devices[params.serial] = device;
    return true;
}

bool DeviceManage::disconnectDevice(const QString &serial) {
    bool ret = false;
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        auto it = m_devices.find(serial);
        if (it->data()) {
            delete it->data();
            ret = true;
        }
    }
    return ret;
}

void DeviceManage::disconnectAllDevice() {
    QMapIterator<QString, QPointer<IDevice>> i(m_devices);
    while (i.hasNext()) {
        i.next();
        if (i.value()) {
            delete i.value();
        }
    }
}

void DeviceManage::onDeviceConnected(bool success,
                                     const QString &serial,
                                     const QString &deviceName,
                                     const QSize &size) {
    emit deviceConnected(success, serial, deviceName, size);
    if (!success) {
        removeDevice(serial);
    }
}

void DeviceManage::onDeviceDisconnected(QString serial) {
    emit deviceDisconnected(serial);
    removeDevice(serial);
}

void DeviceManage::removeDevice(const QString &serial) {
    if (!serial.isEmpty() && m_devices.contains(serial)) {
        m_devices[serial]->deleteLater();
        m_devices.remove(serial);
    }
}

} // namespace qsc
