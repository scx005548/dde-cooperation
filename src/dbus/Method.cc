#include "Method.h"
#include "Interface.h"

namespace DBus {

/*****************************************************************************
 * @brief 构造一个 DBus 方法
 * @param[in] name 方法名
 * @param[in] fn 响应方法调用的回调函数
 * @param [in] inArgs 输入参数列表，是参数名映射到类型字符串的map
 * @param [in] outArgs 返回值列表，是参数名映射到类型字符串的map
 * ***************************************************************************/
Method::Method(const Glib::ustring &name,
               const Callback &fn,
               const std::vector<std::pair<Glib::ustring, Glib::ustring>> &inArgs,
               const std::vector<std::pair<Glib::ustring, Glib::ustring>> &outArgs) noexcept
    : m_name(name)
    , m_callback(fn)
    , m_inArgs(inArgs)
    , m_outArgs(outArgs) {
}

/*****************************************************************************
 * @brief 返回方法名
 * @return 方法名
 * ***************************************************************************/
Glib::ustring Method::name() const noexcept {
    return m_name;
}

/*****************************************************************************
 * @brief 返回 XML 字符串
 * @return xml
 * ***************************************************************************/
Glib::ustring Method::XML() const noexcept {
    Glib::ustring xml = Glib::ustring::compose("    <method name='%1'>\n", m_name);

    for (auto &in : m_inArgs) {
        xml += Glib::ustring::compose("      <arg type='%1' name='%2' direction='in'/>\n",
                                      in.second,
                                      in.first);
    }

    for (auto &out : m_outArgs) {
        xml += Glib::ustring::compose("      <arg type='%1' name='%2' direction='out'/>\n",
                                      out.second,
                                      out.first);
    }

    xml += "    </method>\n";

    return xml;
}

/*****************************************************************************
 * @brief 触发回调函数，DBus 方法调用
 * @param[in] connection DBus连接
 * @param[in] sender 发送方
 * @param[in] objectPath 对象路径
 * @param[in] interfaceName 接口名
 * @param[in] methodName 方法名
 * @param[in] args 参数
 * @param[in] invocation
 * ***************************************************************************/
void Method::onMethodCall(
    const Glib::RefPtr<Gio::DBus::Connection> &connection,
    const Glib::ustring &sender,
    const Glib::ustring &objectPath,
    const Glib::ustring &interfaceName,
    const Glib::ustring &methodName,
    const Glib::VariantContainerBase &args,
    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) const noexcept {
    m_callback(connection, sender, objectPath, interfaceName, methodName, args, invocation);
}

} // namespace DBus
