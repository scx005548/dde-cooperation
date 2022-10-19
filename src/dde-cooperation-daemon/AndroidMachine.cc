#include "AndroidMachine.h"

AndroidMachine::AndroidMachine(Manager *manager,
                               ClipboardBase *clipboard,
                               const std::shared_ptr<uvxx::Loop> &uvLoop,
                               Glib::RefPtr<DBus::Service> service,
                               uint32_t id,
                               const std::filesystem::path &dataDir,
                               const Glib::ustring &ip,
                               uint16_t port,
                               const DeviceInfo &sp)
    : Machine(manager, clipboard, uvLoop, service, id, dataDir, ip, port, sp) {
    init();
}

void AndroidMachine::handleConnected() {
}
