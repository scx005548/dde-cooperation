#include "PCMachine.h"

#include "utils/message_helper.h"
#include "protocol/message.pb.h"

#include "uvxx/TCP.h"
#include "uvxx/Async.h"

PCMachine::PCMachine(Manager *manager,
                     ClipboardBase *clipboard,
                     const std::shared_ptr<uvxx::Loop> &uvLoop,
                     Glib::RefPtr<DBus::Service> service,
                     uint32_t id,
                     const std::filesystem::path &dataDir,
                     const Glib::ustring &ip,
                     uint16_t port,
                     const DeviceInfo &sp)
    : Machine(manager, clipboard, uvLoop, service, id, dataDir, ip, port, sp)
    , m_linuxInterface(new DBus::Interface("com.deepin.Cooperation.Machine.PC"))
    , m_methodMountFs(new DBus::Method("MountFs",
                                       DBus::Method::warp(this, &PCMachine::mountFs),
                                       {{"path", "s"}})) {
    m_linuxInterface->exportMethod(m_methodMountFs);
    m_object->exportInterface(m_linuxInterface);

    init();
}

void PCMachine::mountFs(const Glib::VariantContainerBase &args,
                        const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept {
    if (!m_conn) {
        invocation->return_error(
            Gio::DBus::Error{Gio::DBus::Error::ACCESS_DENIED, "connect first"});
        return;
    }

    Glib::Variant<Glib::ustring> path;
    args.get_child(path, 0);

    m_async->wake([this, path = path.get()]() { mountFs(path); });

    invocation->return_value(Glib::VariantContainerBase{});
}

void PCMachine::handleConnected() {
    mountFs("/");
}

void PCMachine::handleDisconnected() {
}

void PCMachine::mountFs(const std::string &path) {
    Message msg;
    auto *request = msg.mutable_fsrequest();
    request->set_path(path);
    sendMessage(msg);
}
