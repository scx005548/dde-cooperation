#include "Property.h"



namespace DBus
{

/*****************************************************************************
 * @brief 构造一个 DBus 属性
 * @param[in] name 属性名
 * @param[in] type 类型字符串
 * @param [in] getFn get的回调函数
 * @param [in] setFn set的回调函数
 * ***************************************************************************/
Property::Property(const Glib::ustring& name, const Glib::ustring& type,
                        const CallbackGet& getFn, const CallbackSet& setFn):
    m_name(name),
    m_type(type),
    m_get(getFn),
    m_set(setFn)
{

}

/*****************************************************************************
 * @brief 返回属性名
 * @return 属性名
 * ***************************************************************************/
Glib::ustring Property::name() const noexcept
{
    return m_name;
}

/*****************************************************************************
 * @brief 返回属性的类型字符串
 * @return 类型字符串
 * ***************************************************************************/
Glib::ustring Property::type() const noexcept
{
    return m_type;
}

/*****************************************************************************
 * @brief 返回 XML 字符串
 * @return xml
 * ***************************************************************************/
Glib::ustring Property::XML() const noexcept
{
    Glib::ustring access;
    if (m_get != nullptr)
        access += "read";

    if (m_set != nullptr)
        access += "write";

    return Glib::ustring::compose("    <property name='%1' type='%2' access='%3'/>\n", m_name, m_type, access);
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
void Property::onGetProperty(Glib::VariantBase& property,
                                const Glib::RefPtr<Gio::DBus::Connection>& connection,
                                const Glib::ustring& sender,
                                const Glib::ustring& objectPath,
                                const Glib::ustring& interfaceName,
                                const Glib::ustring& propertyName) const noexcept
{
    m_get(property, connection, sender, objectPath, interfaceName, propertyName);
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
bool Property::onSetProperty(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                                const Glib::ustring& sender,
                                const Glib::ustring& objectPath,
                                const Glib::ustring& interfaceName,
                                const Glib::ustring& propertyName,
                                const Glib::VariantBase& value) noexcept
{
    return m_set(connection, sender, objectPath, interfaceName, propertyName, value);
}

}; // namespace DBus
