#ifndef DBUS_OBJECT_H
#define DBUS_OBJECT_H

#include <map>

#include <giomm.h>
#include <glibmm.h>

#include "Interface.h"

namespace DBus {

class Service;

class Object : public Glib::Object {
    friend Service;

public:
    explicit Object(const Glib::ustring &path) noexcept;

    /*****************************************************************************
     * @brief 返回对象路径
     * @return 对象路径
     * ***************************************************************************/
    Glib::ustring path() const noexcept;

    /*****************************************************************************
     * @brief 返回 XML 字符串
     * @return xml
     * ***************************************************************************/
    Glib::ustring XML() const noexcept;

    /*****************************************************************************
     * @brief 导出接口
     * @param[in] service 服务
     * @return id
     * ***************************************************************************/
    bool exportInterface(const Glib::RefPtr<Interface> &interface) noexcept;

    void emitSignal(const Glib::ustring &interface,
                    const Glib::ustring &signal,
                    const Glib::VariantContainerBase &value) noexcept;

protected:
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
    void onMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                      const Glib::ustring &sender,
                      const Glib::ustring &objectPath,
                      const Glib::ustring &interfaceName,
                      const Glib::ustring &methodName,
                      const Glib::VariantContainerBase &args,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) const noexcept;

    /*****************************************************************************
     * @brief 回调函数，DBus 属性读取
     * @param[in] property 属性
     * @param[in] connection 连接
     * @param[in] sender 发送方
     * @param[in] objectPath 对象路径
     * @param[in] interfaceName 接口名
     * @param[in] propertyName 属性名
     * ***************************************************************************/
    void onGetProperty(Glib::VariantBase &property,
                       const Glib::RefPtr<Gio::DBus::Connection> &connection,
                       const Glib::ustring &sender,
                       const Glib::ustring &objectPath,
                       const Glib::ustring &interfaceName,
                       const Glib::ustring &propertyName) const noexcept;

    /*****************************************************************************
     * @brief 回调函数，DBus 属性读取
     * @param[in] connection 连接
     * @param[in] sender 发送方
     * @param[in] objectPath 对象路径
     * @param[in] interfaceName 接口名
     * @param[in] propertyName 属性名
     * @param[in] value 属性值
     * ***************************************************************************/
    bool onSetProperty(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                       const Glib::ustring &sender,
                       const Glib::ustring &objectPath,
                       const Glib::ustring &interfaceName,
                       const Glib::ustring &propertyName,
                       const Glib::VariantBase &value) noexcept;

private:
    Glib::ustring m_path;
    std::map<Glib::ustring, Glib::RefPtr<Interface>> m_interfaces;

protected:
    Service *m_parent;
}; // class Object

} // namespace DBus

#endif // !DBUS_OBJECT_H
