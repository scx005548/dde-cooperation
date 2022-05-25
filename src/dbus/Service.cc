#include "Service.h"

namespace DBus {

/* 服务名 => 服务 */
std::map<Glib::ustring, Glib::RefPtr<Service>> Service::services;

/* 对象路径 => 所属服务 */
std::map<Glib::ustring, Glib::RefPtr<Service>> Service::objServices;

Service::Service(const Glib::ustring &name, Gio::DBus::BusType type)
    : m_name(name)
    , m_type(type)
    , m_vtable{Method::warp(this, &Service::onMethodCall),
               Property::warp(this, &Service::onGetProperty),
               Property::warp(this, &Service::onSetProperty)}
    , m_ownerId(0)
    , m_registered(false) {
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
        update();
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
        update();
        return true;
    }

    return false;
}

/*****************************************************************************
 * @brief 刷新服务，在服务中增删对象、接口、方法、属性、信号时，需要调用这个函数进行更新
 *        并且，只有调用 registerService 注册过的服务才会更新，
 *        调用了 unregisterService 的服务则只会被删除
 * @param[in] service 服务
 * @return id
 * ***************************************************************************/
guint Service::update() {
    if (m_ownerId != 0) {
        Gio::DBus::unown_name(m_ownerId);
        m_ownerId = 0;
    }

    if (m_registered) {
        m_ownerId = Gio::DBus::own_name(m_type,
                                        m_name,
                                        sigc::ptr_fun(&onBusAcquired),
                                        sigc::ptr_fun(&onNameAcquired),
                                        sigc::ptr_fun(&onNameLost));

        return m_ownerId;
    }
    return 0;
}

/*****************************************************************************
 * @brief 注册服务
 * @param[in] service 服务对象
 * @param[in] type 总线类型
 * @return 注册id
 * ***************************************************************************/
guint Service::registerService(const Glib::RefPtr<Service> &service) {
    auto iter = Service::services.find(service->m_name);
    if (iter != services.end()) {
        fprintf(stderr, "conflicting service name '%s'\n", service->m_name.c_str());
        return 0;
    }

    for (auto &obj : service->m_objects) {
        auto path = obj.first;
        if (Service::objServices.find(path) != Service::objServices.end()) {
            fprintf(stderr, "conflicting object path '%s'\n", path.c_str());
            return 0;
        }
    }

    for (auto &obj : service->m_objects) {
        Service::objServices[obj.first] = service;
    }
    Service::services[service->m_name] = service;

    service->m_registered = true;
    return service->update();
}

/*****************************************************************************
 * @brief 删除服务
 * @param[in] name 名字
 * @return 是否成功
 * ***************************************************************************/
bool Service::unregisterService(const Glib::ustring &name) {
    auto iter = Service::services.find(name);
    if (iter == Service::services.end()) {
        return false;
    }

    auto service = iter->second;
    Service::services.erase(iter);

    for (auto &obj : service->m_objects) {
        Service::objServices.erase(obj.first);
    }

    service->m_registered = false;
    service->update();
    return true;
}

/*****************************************************************************
 * @brief 回调函数，DBus 获得总线
 * @param[in] connection DBus连接
 * @param[in] name 服务名
 * ***************************************************************************/
void Service::onBusAcquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                            const Glib::ustring &name) {
    try {
        auto service = Service::services.at(name);

        for (auto iter : service->m_objects) {
            auto name = iter.first;
            auto obj = iter.second;

            for (auto iter2 : obj->m_interfaces) {
                auto introspectionData = Gio::DBus::NodeInfo::create_for_xml(iter2.second->XML());
                service->m_objIds[name].emplace_back(
                    connection->register_object(obj->path(),
                                                introspectionData->lookup_interface(),
                                                service->m_vtable));
            }
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
        auto service = Service::services.at(name);
        for (auto id : service->m_objIds[name]) {
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
