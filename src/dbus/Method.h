#ifndef DBUS_METHOD_H
#define DBUS_METHOD_H

#include <map>
#include <vector>
#include <functional>

#include <giomm.h>
#include <glibmm.h>

namespace DBus {

class Interface;

class Method : public Glib::Object {
    friend Interface;

public:
    // 回调函数的类型
    using Callback = std::function<void(
        const Glib::RefPtr<Gio::DBus::Connection> &connection,
        const Glib::ustring &sender,
        const Glib::ustring &objectPath,
        const Glib::ustring &interfaceName,
        const Glib::ustring &methodName,
        const Glib::VariantContainerBase &args,
        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation)>;

    /* 完整方法的成员函数指针类型 */
    template <class C>
    using MethodFunc = void (C::*)(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                   const Glib::ustring &sender,
                                   const Glib::ustring &objectPath,
                                   const Glib::ustring &interfaceName,
                                   const Glib::ustring &methodName,
                                   const Glib::VariantContainerBase &args,
                                   const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

    /* 完整方法的普通函数类型 */
    using PlainFunc = Callback;

    /* 简单方法的成员函数指针类型 */
    template <class C>
    using SimpleMethodFunc =
        void (C::*)(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation);

    /* 简单方法的普通函数类型 */
    using SimplePlainFunc = std::function<void(
        const Glib::VariantContainerBase &args,
        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation)>;

    /*****************************************************************************
     * @brief 构造一个 DBus 方法
     * @param[in] name 方法名
     * @param[in] fn 响应方法调用的回调函数
     * @param [in] inArgs 输入参数列表，是参数名映射到类型字符串的map
     * @param [in] outArgs 返回值列表，是参数名映射到类型字符串的map
     * ***************************************************************************/
    explicit Method(const Glib::ustring &name,
                    const Callback &fn,
                    const std::map<Glib::ustring, Glib::ustring> &inArgs = {},
                    const std::map<Glib::ustring, Glib::ustring> &outArgs = {}) noexcept;

    /*****************************************************************************
     * @brief 返回方法名
     * @return 方法名
     * ***************************************************************************/
    Glib::ustring name() const noexcept;

    /*****************************************************************************
     * @brief 返回 XML 字符串
     * @return xml
     * ***************************************************************************/
    Glib::ustring XML() const noexcept;

    /*****************************************************************************
     * @brief 封装完整参数的成员函数
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的方法函数
     * ***************************************************************************/
    template <class C>
    static Callback warp(C *self, const MethodFunc<C> &fn) noexcept {
        return [self, fn](const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &sender,
                          const Glib::ustring &objectPath,
                          const Glib::ustring &interfaceName,
                          const Glib::ustring &methodName,
                          const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {
            (self->*fn)(connection,
                        sender,
                        objectPath,
                        interfaceName,
                        methodName,
                        args,
                        invocation);
        };
    }

    /*****************************************************************************
     * @brief 封装完整参数的普通函数
     * @param[in] fn 普通函数
     * @return 封装后的方法函数
     * ***************************************************************************/
    static Callback warp(const PlainFunc &fn) noexcept { return fn; }

    /*****************************************************************************
     * @brief 封装简化参数的成员函数
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的方法函数
     * ***************************************************************************/
    template <class C>
    static Callback warp(C *self, const SimpleMethodFunc<C> &fn) noexcept {
        return [self, fn](const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &sender,
                          const Glib::ustring &objectPath,
                          const Glib::ustring &interfaceName,
                          const Glib::ustring &methodName,
                          const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {
            (self->*fn)(args, invocation);
        };
    }

    /*****************************************************************************
     * @brief 封装简化参数的普通函数
     * @param[in] fn 普通函数
     * @return 封装后的方法函数
     * ***************************************************************************/
    static Callback warp(const SimplePlainFunc &fn) noexcept {
        return [fn](const Glib::RefPtr<Gio::DBus::Connection> &connection,
                    const Glib::ustring &sender,
                    const Glib::ustring &objectPath,
                    const Glib::ustring &interfaceName,
                    const Glib::ustring &methodName,
                    const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) {
            fn(args, invocation);
        };
    }

protected:
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
    void onMethodCall(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                      const Glib::ustring &sender,
                      const Glib::ustring &objectPath,
                      const Glib::ustring &interfaceName,
                      const Glib::ustring &methodName,
                      const Glib::VariantContainerBase &args,
                      const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) const noexcept;

private:
    Glib::ustring m_name; // 方法名
    Callback m_callback;  // 回调函数

    // 参数名 => 类型字符串
    std::map<Glib::ustring, Glib::ustring> m_inArgs;
    std::map<Glib::ustring, Glib::ustring> m_outArgs;
};

}; // namespace DBus

#endif // !DBUS_METHOD_H
