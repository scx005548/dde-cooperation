#include "Object.h"
#include "Service.h"

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
 * @brief 刷新所属服务
 * @return id
 * ***************************************************************************/
guint Object::update() {
    if (m_parent != nullptr) return m_parent->update();

    return 0;
}

/*****************************************************************************
 * @brief 导出接口
 * @param[in] service 服务
 * @return id
 * ***************************************************************************/
bool Object::exportInterface(const Glib::RefPtr<Interface> &interface) noexcept {
    if (interface->m_parent != nullptr) {
        fprintf(stderr, "%s already have a parent\n", interface->name().c_str());
        return false;
    }

    auto iter = m_interfaces.find(interface->name());
    if (iter == m_interfaces.end()) {
        interface->m_parent = this;
        m_interfaces[interface->name()] = interface;
        update();
        return true;
    }

    return false;
}

/*****************************************************************************
 * @brief 删除接口
 * @param[in] service 服务
 * @return id
 * ***************************************************************************/
bool Object::unexportInterface(const Glib::ustring &name) noexcept {
    auto iter = m_interfaces.find(name);
    if (iter != m_interfaces.end()) {
        iter->second->m_parent = nullptr;
        m_interfaces.erase(iter);
        update();
        return true;
    }

    return false;
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

}; // namespace DBus
