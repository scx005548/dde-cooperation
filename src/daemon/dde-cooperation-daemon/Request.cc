#include "Request.h"

Request::Request(const Glib::RefPtr<DBus::Service> &service,
                 uint32_t id,
                 Type type,
                 uint32_t serial)
    : m_serial(serial)
    , m_service(service)
    , m_object(new DBus::Object(Glib::ustring::compose("/com/deepin/Cooperation/Request/%1", id)))
    , m_interface(new DBus::Interface("com.deepin.Cooperation.Request"))
    , m_methodAccept(new DBus::Method("Accept",
                                      DBus::Method::warp(this, &Request::accept),
                                      {{"accept", "b"}, {"hint", "a{sv}"}}))
    , m_type(type)
    , m_propertyType(new DBus::Property("Type", "q", DBus::Property::warp(this, &Request::getType)))
    , m_propertyMachine(
          new DBus::Property("Machine", "o", DBus::Property::warp(this, &Request::getMachine))) {
    m_interface->exportMethod(m_methodAccept);
    m_interface->exportProperty(m_propertyType);
    m_object->exportInterface(m_interface);
    m_service->exportObject(m_object);
}

Request::~Request() {
    m_service->unexportObject(m_object->path());
}

Glib::DBusObjectPathString Request::path() const {
    return Glib::DBusObjectPathString(m_object->path());
}

void Request::accept([[maybe_unused]] const Glib::VariantContainerBase &args,
                     const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    invocation->return_value(Glib::VariantContainerBase{});

    Glib::Variant<bool> accepted;
    Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>> hint;
    args.get_child(accepted, 0);
    args.get_child(hint, 1);

    m_signal_onAccept.emit(accepted.get(), hint.get(), m_serial);
}

void Request::getType(Glib::VariantBase &property,
                      [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<uint16_t>::create(static_cast<uint16_t>(m_type));
}

void Request::getMachine(Glib::VariantBase &property,
                         [[maybe_unused]] const Glib::ustring &propertyName) const noexcept {
    property = Glib::Variant<Glib::DBusObjectPathString>::create(m_machinePath);
}
