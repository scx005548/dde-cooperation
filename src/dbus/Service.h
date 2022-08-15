#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <map>

#include <giomm.h>
#include <glibmm.h>

#include "Object.h"

namespace DBus {

class Object;

class Service : public Glib::Object {
public:
    explicit Service(const Glib::ustring &name,
                     Gio::DBus::BusType type = Gio::DBus::BusType::BUS_TYPE_SESSION);
    explicit Service(Glib::RefPtr<Gio::DBus::Connection> conn);
    ~Service() = default;

    Glib::RefPtr<Gio::DBus::Connection> conn() const noexcept;

    /*****************************************************************************
     * @brief 返回服务名
     * @return 服务名
     * ***************************************************************************/
    Glib::ustring name() const noexcept;

    /*****************************************************************************
     * @brief 导出对象
     * @param[in] obj 对象
     * @return 是否成功
     * ***************************************************************************/
    bool exportObject(const Glib::RefPtr<DBus::Object> &obj) noexcept;

    /*****************************************************************************
     * @brief 删除对象
     * @param[in] path 对象路径
     * @return 是否成功
     * ***************************************************************************/
    bool unexportObject(const Glib::ustring &path) noexcept;

    /*****************************************************************************
     * @brief 注册服务
     * @param[in] service 服务
     * @return id
     * ***************************************************************************/
    guint registerService();

    /*****************************************************************************
     * @brief 删除服务
     * @param[in] name 名字
     * @return 是否成功
     * ***************************************************************************/
    bool unregisterService();

    void emitSignal(const Glib::ustring &service,
                    const Glib::ustring &interface,
                    const Glib::ustring &signal,
                    const Glib::VariantContainerBase &value) noexcept;

private:
    Glib::ustring m_name;
    Gio::DBus::BusType m_type;

    Glib::RefPtr<Gio::DBus::Connection> m_conn;
    Gio::DBus::InterfaceVTable m_vtable; // DBus 虚表
    guint m_ownerId;

    std::map<Glib::ustring, Glib::RefPtr<DBus::Object>> m_objects;
    std::map<Glib::ustring, std::vector<guint>> m_objIds;

    void exportObjectAux(const Glib::RefPtr<DBus::Object> &obj);

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
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

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

    /* 服务名 => 服务 */
    static std::map<Glib::ustring, Glib::RefPtr<Service>> services;

    /* 对象路径 => 所属服务 */
    static std::map<Glib::ustring, Glib::RefPtr<Service>> objServices;

    /*****************************************************************************
     * @brief 回调函数，DBus 获得总线
     * @param[in] connection DBus连接
     * @param[in] name 名字
     * ***************************************************************************/
    void onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                       const Glib::ustring &name);

    /*****************************************************************************
     * @brief 回调函数，DBus 获得连接名
     * @param[in] connection DBus连接
     * @param[in] name 名字
     * ***************************************************************************/
    void onNameAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                        const Glib::ustring &name);

    /*****************************************************************************
     * @brief 回调函数，DBus 失去连接名
     * @param[in] connection DBus连接
     * @param[in] name 名字
     * ***************************************************************************/
    void onNameLost(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                    const Glib::ustring &name);
};

} // namespace DBus

#endif // !DBUS_SERVICE_H
