#ifndef DBUS_PROPERTY_H
#define DBUS_PROPERTY_H

#include <map>

#include <giomm.h>
#include <glibmm.h>

namespace DBus {

class Interface;

class Property : public Glib::Object {
    friend Interface;

public:
    // Get回调函数类型
    using CallbackGet = std::function<void(Glib::VariantBase &property,
                                           const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                           const Glib::ustring &sender,
                                           const Glib::ustring &objectPath,
                                           const Glib::ustring &interfaceName,
                                           const Glib::ustring &propertyName)>;

    // Set回调函数类型
    using CallbackSet = std::function<bool(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                           const Glib::ustring &sender,
                                           const Glib::ustring &objectPath,
                                           const Glib::ustring &interfaceName,
                                           const Glib::ustring &propertyName,
                                           const Glib::VariantBase &value)>;

    // 完整参数的成员函数指针类型
    template <class C>
    using MethodFuncGet = void (C::*)(Glib::VariantBase &property,
                                      const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                      const Glib::ustring &sender,
                                      const Glib::ustring &objectPath,
                                      const Glib::ustring &interfaceName,
                                      const Glib::ustring &propertyName) const;

    template <class C>
    using MethodFuncSet = bool (C::*)(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                      const Glib::ustring &sender,
                                      const Glib::ustring &objectPath,
                                      const Glib::ustring &interfaceName,
                                      const Glib::ustring &propertyName,
                                      const Glib::VariantBase &value);

    // 完整参数的普通函数类型
    using PlainFuncGet = CallbackGet;
    using PlainFuncSet = CallbackSet;

    // 简化参数的成员函数指针类型
    template <class C>
    using SimpleMethodFuncGet = void (C::*)(Glib::VariantBase &property,
                                            const Glib::ustring &propertyName) const;

    template <class C>
    using SimpleMethodFuncSet = bool (C::*)(const Glib::ustring &propertyName,
                                            const Glib::VariantBase &value);

    // 简化参数的普通函数类型
    using SimplePlainFuncGet = std::function<void(Glib::VariantBase &property,
                                                  const Glib::ustring &propertyName)>;
    using SimplePlainFuncSet = std::function<void(const Glib::ustring &propertyName,
                                                  const Glib::VariantBase &value)>;

    /*****************************************************************************
     * @brief 构造一个 DBus 属性
     * @param[in] name 属性名
     * @param[in] type 类型字符串
     * @param[in] getFn get的回调函数
     * @param[in] setFn set的回调函数
     * ***************************************************************************/
    explicit Property(const Glib::ustring &name,
                      const Glib::ustring &type,
                      const CallbackGet &getFn = nullptr,
                      const CallbackSet &setFn = nullptr);

    /*****************************************************************************
     * @brief 返回属性名
     * @return 属性名
     * ***************************************************************************/
    Glib::ustring name() const noexcept;

    /*****************************************************************************
     * @brief 返回属性的类型字符串
     * @return 类型字符串
     * ***************************************************************************/
    Glib::ustring type() const noexcept;

    /*****************************************************************************
     * @brief 返回 XML 字符串
     * @return xml
     * ***************************************************************************/
    Glib::ustring XML() const noexcept;

    void emitChanged(const Glib::VariantBase &value) noexcept;

    /*****************************************************************************
     * @brief 封装完整参数的成员函数作为Get
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的属性Get函数
     * ***************************************************************************/
    template <class C>
    static CallbackGet warp(const C *self, const MethodFuncGet<C> &fn) noexcept {
        return [self, fn](Glib::VariantBase &property,
                          const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &sender,
                          const Glib::ustring &objectPath,
                          const Glib::ustring &interfaceName,
                          const Glib::ustring &propertyName) {
            return (
                self->*fn)(property, connection, sender, objectPath, interfaceName, propertyName);
        };
    }

    /*****************************************************************************
     * @brief 封装完整参数的成员函数作为Set
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的属性Set函数
     * ***************************************************************************/
    template <class C>
    static CallbackSet warp(C *self, const MethodFuncSet<C> &fn) noexcept {
        return [self, fn](const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          const Glib::ustring &sender,
                          const Glib::ustring &objectPath,
                          const Glib::ustring &interfaceName,
                          const Glib::ustring &propertyName,
                          const Glib::VariantBase &value) {
            return (self->*fn)(connection, sender, objectPath, interfaceName, propertyName, value);
        };
    }

    /*****************************************************************************
     * @brief 封装完整参数的普通函数作为Get
     * @param[in] fn 函数
     * @return 封装后的属性Get函数
     * ***************************************************************************/
    template <class C>
    static CallbackGet warp(const PlainFuncGet &fn) noexcept {
        return fn;
    }

    /*****************************************************************************
     * @brief 封装完整参数的普通函数作为Set
     * @param[in] fn 函数
     * @return 封装后的属性Set函数
     * ***************************************************************************/
    template <class C>
    static CallbackSet warp(const PlainFuncSet &fn) noexcept {
        return fn;
    }

    /*****************************************************************************
     * @brief 封装简化参数的成员函数作为Get
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的属性Get函数
     * ***************************************************************************/
    template <class C>
    static CallbackGet warp(const C *self, const SimpleMethodFuncGet<C> &fn) noexcept {
        return [self, fn](Glib::VariantBase &property,
                          [[maybe_unused]] const Glib::RefPtr<Gio::DBus::Connection> &connection,
                          [[maybe_unused]] const Glib::ustring &sender,
                          [[maybe_unused]] const Glib::ustring &objectPath,
                          [[maybe_unused]] const Glib::ustring &interfaceName,
                          const Glib::ustring &propertyName) {
            return (self->*fn)(property, propertyName);
        };
    }

    /*****************************************************************************
     * @brief 封装简化参数的成员函数作为Set
     * @param[in] self 对象指针
     * @param[in] fn 成员函数
     * @return 封装后的属性Set函数
     * ***************************************************************************/
    template <class C>
    static CallbackSet warp(C *self, const SimpleMethodFuncSet<C> &fn) noexcept {
        return
            [self, fn]([[maybe_unused]] const Glib::RefPtr<Gio::DBus::Connection> &connection,
                       [[maybe_unused]] const Glib::ustring &sender,
                       [[maybe_unused]] const Glib::ustring &objectPath,
                       [[maybe_unused]] const Glib::ustring &interfaceName,
                       const Glib::ustring &propertyName,
                       const Glib::VariantBase &value) { return (self->*fn)(propertyName, value); };
    }

    /*****************************************************************************
     * @brief 封装简化参数的普通函数作为Get
     * @param[in] fn 函数
     * @return 封装后的属性Get函数
     * ***************************************************************************/
    template <class C>
    static CallbackGet warp(const SimplePlainFuncGet &fn) noexcept {
        return [fn](Glib::VariantBase &property,
                    [[maybe_unused]] const Glib::RefPtr<Gio::DBus::Connection> &connection,
                    [[maybe_unused]] const Glib::ustring &sender,
                    [[maybe_unused]] const Glib::ustring &objectPath,
                    [[maybe_unused]] const Glib::ustring &interfaceName,
                    const Glib::ustring &propertyName) { return fn(property, propertyName); };
    }

    /*****************************************************************************
     * @brief 封装简化参数的普通函数作为Set
     * @param[in] fn 函数
     * @return 封装后的属性Set函数
     * ***************************************************************************/
    template <class C>
    static CallbackSet warp(const SimplePlainFuncSet &fn) noexcept {
        return [fn]([[maybe_unused]] const Glib::RefPtr<Gio::DBus::Connection> &connection,
                    [[maybe_unused]] const Glib::ustring &sender,
                    [[maybe_unused]] const Glib::ustring &objectPath,
                    [[maybe_unused]] const Glib::ustring &interfaceName,
                    const Glib::ustring &propertyName,
                    const Glib::VariantBase &value) { return fn(propertyName, value); };
    }

private:
    Glib::ustring m_name;
    Glib::ustring m_type;
    CallbackGet m_get;
    CallbackSet m_set;

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

protected:
    Interface *m_parent;

}; // class Property

} // namespace DBus

#endif // !DBUS_PROPERTY_H
