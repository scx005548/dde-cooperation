#include "Service.h"

namespace DBus {

Service::Service(const Glib::ustring &name, Gio::DBus::BusType type)
    : m_name(name)
    , m_type(type)
    , m_conn(nullptr)
    , m_vtable{Method::warp(this, &Service::onMethodCall),
               Property::warp(this, &Service::onGetProperty),
               Property::warp(this, &Service::onSetProperty)}
    , m_ownerId(0) {
}

Service::Service(Glib::RefPtr<Gio::DBus::Connection> conn)
    : m_conn(conn)
    , m_vtable{Method::warp(this, &Service::onMethodCall),
               Property::warp(this, &Service::onGetProperty),
               Property::warp(this, &Service::onSetProperty)}
    , m_ownerId(0) {
}

/*****************************************************************************
 * @brief 返回服务名
 * @return 服务名
 * ***************************************************************************/
Glib::ustring Service::name() const noexcept {
    return m_name;
}

/*****************************************************************************
 * @brief 导出对象
 * @param[in] obj 对象
 * @return 是否成功
 * ***************************************************************************/
bool Service::exportObject(const Glib::RefPtr<DBus::Object> &obj) noexcept {
    if (obj->m_parent != nullptr) {
        fprintf(stderr, "%s already have a parent\n", obj->path().c_str());
        return false;
    }

    auto iter = m_objects.find(obj->path());
    if (iter == m_objects.end()) {
        obj->m_parent = this;
        m_objects[obj->path()] = obj;

        if (m_conn) {
            exportObjectAux(obj);
        }

        return true;
    }

    return false;
}

/*****************************************************************************
 * @brief 删除对象
 * @param[in] path 对象路径
 * @return 是否成功
 * ***************************************************************************/
bool Service::unexportObject(const Glib::ustring &path) noexcept {
    auto iter = m_objects.find(path);
    if (iter != m_objects.end()) {
        iter->second->m_parent = nullptr;
        m_objects.erase(iter);

        if (m_conn) {
            auto ids = m_objIds.at(path);
            for (auto id : ids) {
                m_conn->unregister_object(id);
            }
        }

        return true;
    }

    return false;
}

/*****************************************************************************
 * @brief 注册服务
 * @param[in] service 服务对象
 * @param[in] type 总线类型
 * @return 注册id
 * ***************************************************************************/
guint Service::registerService() {
    return Gio::DBus::own_name(m_type,
                               m_name,
                               sigc::mem_fun(this, &Service::onBusAcquired),
                               sigc::mem_fun(this, &Service::onNameAcquired),
                               sigc::mem_fun(this, &Service::onNameLost));
}

/*****************************************************************************
 * @brief 删除服务
 * @param[in] name 名字
 * @return 是否成功
 * ***************************************************************************/
bool Service::unregisterService() {
    Gio::DBus::unown_name(m_ownerId);
    m_ownerId = 0;

    return true;
}

void Service::exportObjectAux(const Glib::RefPtr<DBus::Object> &obj) {
    for (auto iter : obj->m_interfaces) {
        auto introspectionData = Gio::DBus::NodeInfo::create_for_xml(iter.second->XML());
        m_objIds[obj->path()].emplace_back(
            m_conn->register_object(obj->path(), introspectionData->lookup_interface(), m_vtable));
    }
}

/*****************************************************************************
 * @brief 回调函数，DBus 获得总线
 * @param[in] connection DBus连接
 * @param[in] name 服务名
 * ***************************************************************************/
void Service::onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                            [[maybe_unused]] const Glib::ustring &name) {
    try {
        m_conn = connection;
        for (auto iter : m_objects) {
            exportObjectAux(iter.second);
        }
    } catch (const std::exception &err) {
        fprintf(stderr, "onBusAcquired: %s\n", err.what());
    }
}

/*****************************************************************************
 * @brief 回调函数，DBus 获得连接名
 * @param[in] connection DBus连接
 * @param[in] name 名字
 * ***************************************************************************/
void Service::onNameAcquired([[maybe_unused]] const Glib::RefPtr<Gio::DBus::Connection> &connection,
                             [[maybe_unused]] const Glib::ustring &name) {
    // TODO: See https://bugzilla.gnome.org/show_bug.cgi?id=646427
}

/*****************************************************************************
 * @brief 回调函数，DBus 失去连接名
 * @param[in] connection DBus连接
 * @param[in] name 名字
 * ***************************************************************************/
void Service::onNameLost(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                         const Glib::ustring &name) {
    try {
        for (auto id : m_objIds[name]) {
            connection->unregister_object(id);
        }
    } catch (const std::exception &err) {
        fprintf(stderr, "onNameLost: %s\n", err.what());
    }
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
void Service::onMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                           const Glib::ustring &sender,
                           const Glib::ustring &objectPath,
                           const Glib::ustring &interfaceName,
                           const Glib::ustring &methodName,
                           const Glib::VariantContainerBase &args,
                           const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {
    auto iter = m_objects.find(objectPath);
    if (iter == m_objects.end()) {
        Gio::DBus::Error e{Gio::DBus::Error::UNKNOWN_OBJECT, objectPath};
        invocation->return_error(e);
        return;
    }
    auto obj = iter->second;
    obj->onMethodCall(connection, sender, objectPath, interfaceName, methodName, args, invocation);
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
void Service::onGetProperty(Glib::VariantBase &property,
                            const Glib::RefPtr<Gio::DBus::Connection> &connection,
                            const Glib::ustring &sender,
                            const Glib::ustring &objectPath,
                            const Glib::ustring &interfaceName,
                            const Glib::ustring &propertyName) const noexcept {
    auto iter = m_objects.find(objectPath);
    if (iter != m_objects.end()) {
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
bool Service::onSetProperty(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                            const Glib::ustring &sender,
                            const Glib::ustring &objectPath,
                            const Glib::ustring &interfaceName,
                            const Glib::ustring &propertyName,
                            const Glib::VariantBase &value) noexcept {
    auto iter = m_objects.find(objectPath);
    if (iter != m_objects.end()) {
        return iter->second
            ->onSetProperty(connection, sender, objectPath, interfaceName, propertyName, value);
    }

    return false;
}

}; // namespace DBus
