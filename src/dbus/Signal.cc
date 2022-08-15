#include "Signal.h"

#include <spdlog/spdlog.h>

#include "Interface.h"

namespace DBus {

/*****************************************************************************
 * @brief 构造一个 DBus 信号
 * @param[in] name 信号名
 * @param[in] type 类型字符串
 * ***************************************************************************/
Signal::Signal(const Glib::ustring &name, const std::map<Glib::ustring, Glib::ustring> &outArgs)
    : m_name(name)
    , m_outArgs(outArgs)
    , m_parent(nullptr) {
    m_type.reserve(2 + m_outArgs.size());
    m_type = "(";
    for (auto &out : m_outArgs) {
        m_type += out.second;
    }
    m_type += ")";
}

/*****************************************************************************
 * @brief 返回信号名
 * @return 信号名
 * ***************************************************************************/
Glib::ustring Signal::name() const noexcept {
    return m_name;
}

/*****************************************************************************
 * @brief 返回信号的类型字符串
 * @return 类型字符串
 * ***************************************************************************/
Glib::ustring Signal::type() const noexcept {
    return m_type;
}

/*****************************************************************************
 * @brief 返回 XML 字符串
 * @return xml
 * ***************************************************************************/
Glib::ustring Signal::XML() const noexcept {
    Glib::ustring xml = Glib::ustring::compose("    <signal name='%1'>\n", m_name);

    for (auto &out : m_outArgs) {
        xml += Glib::ustring::compose("      <arg type='%1' name='%2' direction='out'/>\n",
                                      out.second,
                                      out.first);
    }

    xml += "    </signal>\n";

    return xml;
}

void Signal::emit(const Glib::VariantContainerBase &value) noexcept {
    if (value.get_type_string() != m_type) {
        spdlog::error("wrong type, expect {}, received {}",
                      std::string(m_type),
                      value.get_type_string());
        return;
    }

    m_parent->emitSignal(m_name, value);
}

} // namespace DBus
