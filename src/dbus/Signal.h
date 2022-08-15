#ifndef DBUS_SIGNAL_H
#define DBUS_SIGNAL_H

#include <map>

#include <giomm.h>
#include <glibmm.h>

#include "Interface.h"

namespace DBus {

class Interface;

class Signal : public Glib::Object {
    friend Interface;

public:
    /*****************************************************************************
     * @brief 构造一个 DBus 信号
     * @param[in] name 信号名
     * @param [in] outArgs 输入参数列表，是参数名映射到类型字符串的map
     * ***************************************************************************/
    explicit Signal(const Glib::ustring &name,
                    const std::map<Glib::ustring, Glib::ustring> &outArgs = {});

    /*****************************************************************************
     * @brief 返回信号名
     * @return 信号名
     * ***************************************************************************/
    Glib::ustring name() const noexcept;

    /*****************************************************************************
     * @brief 返回信号的类型字符串
     * @return 类型字符串
     * ***************************************************************************/
    Glib::ustring type() const noexcept;

    /*****************************************************************************
     * @brief 返回 XML 字符串
     * @return xml
     * ***************************************************************************/
    Glib::ustring XML() const noexcept;

    void emit(const Glib::VariantContainerBase &value) noexcept;

private:
    Glib::ustring m_name;
    Glib::ustring m_type;

    // 参数名 => 类型字符串
    std::map<Glib::ustring, Glib::ustring> m_outArgs;

protected:
    DBus::Interface *m_parent;

}; // class Signal

} // namespace DBus

#endif // !DBUS_SIGNAL_H
