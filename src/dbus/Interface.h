#ifndef DBUS_INTERFACE_H
#define DBUS_INTERFACE_H

#include <map>

#include <giomm.h>
#include <glibmm.h>

#include "Method.h"
#include "Property.h"



namespace DBus
{

class Object;

class Interface : public Glib::Object
{
    friend DBus::Object;
public:
    explicit Interface(const Glib::ustring& name) noexcept;

    /*****************************************************************************
     * @brief 返回接口名
     * @return 接口名
     * ***************************************************************************/
    Glib::ustring name() const noexcept;

    /*****************************************************************************
     * @brief 返回 XML 字符串
     * @return xml
     * ***************************************************************************/
    Glib::ustring XML() const noexcept;

    /*****************************************************************************
     * @brief 刷新所属服务
     * @return id
     * ***************************************************************************/
    guint update();

    /*****************************************************************************
     * @brief 导出方法
     * @param[in] method 方法
     * @return 是否成功 
     * ***************************************************************************/
    bool exportMethod(const Glib::RefPtr<Method>& method) noexcept;

    /*****************************************************************************
     * @brief 删除方法
     * @param[in] name 方法名
     * @return 是否成功 
     * ***************************************************************************/
    bool unexportMethod(const Glib::ustring& name) noexcept;

    /*****************************************************************************
     * @brief 导出属性
     * @param[in] property 属性
     * @return 是否成功 
     * ***************************************************************************/
    bool exportProperty(const Glib::RefPtr<Property>& property) noexcept;

    /*****************************************************************************
     * @brief 删除属性
     * @param[in] name 属性名
     * @return 是否成功 
     * ***************************************************************************/
    bool unexportProperty(const Glib::ustring& name) noexcept;

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
    void onMethodCall(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                        const Glib::ustring& sender,
                        const Glib::ustring& objectPath,
                        const Glib::ustring& interfaceName,
                        const Glib::ustring& methodName,
                        const Glib::VariantContainerBase& args,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) const noexcept;

    /*****************************************************************************
     * @brief 回调函数，DBus 属性读取
     * @param[in] property 属性
     * @param[in] connection 连接
     * @param[in] sender 发送方
     * @param[in] objectPath 对象路径
     * @param[in] interfaceName 接口名
     * @param[in] propertyName 属性名
     * ***************************************************************************/
    void onGetProperty(Glib::VariantBase& property,
                        const Glib::RefPtr<Gio::DBus::Connection>& connection,
                        const Glib::ustring& sender,
                        const Glib::ustring& objectPath,
                        const Glib::ustring& interfaceName,
                        const Glib::ustring& propertyName) const noexcept;

    /*****************************************************************************
     * @brief 回调函数，DBus 属性读取
     * @param[in] connection 连接
     * @param[in] sender 发送方
     * @param[in] objectPath 对象路径
     * @param[in] interfaceName 接口名
     * @param[in] propertyName 属性名
     * @param[in] value 属性值
     * ***************************************************************************/
    bool onSetProperty(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                        const Glib::ustring& sender,
                        const Glib::ustring& objectPath,
                        const Glib::ustring& interfaceName,
                        const Glib::ustring& propertyName,
                        const Glib::VariantBase& value) noexcept;
    
private:
    Glib::ustring m_name;
    std::map<Glib::ustring, Glib::RefPtr<Method>> m_methods;        // 方法函数表
    std::map<Glib::ustring, Glib::RefPtr<Property>> m_properties;    // 属性表

protected:
    DBus::Object* m_parent;

}; // class Interface

}; // namespace DBus

#endif // DBUS_Interface_H