#include "Object.h"
#include "Service.h"

#include <cassert>

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;

namespace DBus {

Object::Object(const Glib::ustring &path) noexcept
    : m_path(path)
    , m_parent(nullptr) {
}

/*****************************************************************************
 * @brief 返回对象路径
 * @return 对象路径
 * ***************************************************************************/
Glib::ustring Object::path() const noexcept {
    return m_path;
}

/*****************************************************************************
 * @brief 返回 XML 字符串
 * @return xml
 * ***************************************************************************/
Glib::ustring Object::XML() const noexcept {
    Glib::ustring xml;
    for (auto &i : m_interfaces) {
        xml += i.second->XML();
    }
    return xml;
}

/*****************************************************************************
 * @brief 导出接口，必须在对象导出之前调用
 * @param[in] service 服务
 * @return id
 * ***************************************************************************/
bool Object::exportInterface(const Glib::RefPtr<Interface> &interface) noexcept {
    if (interface->m_parent != nullptr) {
        logger->error("{} already have a parent", std::string(interface->name()));
        return false;
    }

    assert(m_parent == nullptr);

    auto iter = m_interfaces.find(interface->name());
    if (iter == m_interfaces.end()) {
        interface->m_parent = this;
        m_interfaces[interface->name()] = interface;
        return true;
    }

    return false;
}

void Object::emitSignal(const Glib::ustring &interface,
                        const Glib::ustring &signal,
                        const Glib::VariantContainerBase &value) noexcept {
    m_parent->emitSignal(m_path, interface, signal, value);
}

/*****************************************************************************
 * @brief 回调函数，DBus 方法调用
 * @param[in] connection DBus连接
 * @param[in] sender 发送方
 * @param[in] objectPath 对象路径
 * @param[in] interfaceName 接口名
 * @param[in] methodName 方法名
 * @param[in] args 参数
 * @param[in] invocation
 * ***************************************************************************/
void Object::onMethodCall(
    const Glib::RefPtr<Gio::DBus::Connection> &connection,
    const Glib::ustring &sender,
    const Glib::ustring &objectPath,
    const Glib::ustring &interfaceName,
    const Glib::ustring &methodName,
    const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) const noexcept {
    auto iter = m_interfaces.find(interfaceName);
    if (iter == m_interfaces.end()) {
        Gio::DBus::Error err{Gio::DBus::Error::UNKNOWN_INTERFACE, interfaceName};
        invocation->return_error(err);
        return;
    }

    iter->second
        ->onMethodCall(connection, sender, objectPath, interfaceName, methodName, args, invocation);
}

/*****************************************************************************
 * @brief 回调函数，DBus 属性读取
 * @param[in] property 属性
 * @param[in] connection 连接
 * @param[in] sender 发送方
 * @param[in] objectPath 对象路径
 * @param[in] interfaceName 接口名
 * @param[in] propertyName 属性名
 * ***************************************************************************/
void Object::onGetProperty(Glib::VariantBase &property,
                           const Glib::RefPtr<Gio::DBus::Connection> &connection,
                           const Glib::ustring &sender,
                           const Glib::ustring &objectPath,
                           const Glib::ustring &interfaceName,
                           const Glib::ustring &propertyName) const noexcept {
    auto iter = m_interfaces.find(interfaceName);
    if (iter != m_interfaces.end()) {
        iter->second
            ->onGetProperty(property, connection, sender, objectPath, interfaceName, propertyName);
    }
}

/*****************************************************************************
 * @brief 回调函数，DBus 属性读取
 * @param[in] connection 连接
 * @param[in] sender 发送方
 * @param[in] objectPath 对象路径
 * @param[in] interfaceName 接口名
 * @param[in] propertyName 属性名
 * @param[in] value 属性值
 * ***************************************************************************/
bool Object::onSetProperty(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                           const Glib::ustring &sender,
                           const Glib::ustring &objectPath,
                           const Glib::ustring &interfaceName,
                           const Glib::ustring &propertyName,
                           const Glib::VariantBase &value) noexcept {
    auto iter = m_interfaces.find(interfaceName);
    if (iter != m_interfaces.end()) {
        return iter->second
            ->onSetProperty(connection, sender, objectPath, interfaceName, propertyName, value);
    }

    return false;
}

} // namespace DBus
