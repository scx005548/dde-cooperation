#include "PCMachine.h"

#include "utils/message_helper.h"
#include "protocol/message.pb.h"

#include "uvxx/TCP.h"
#include "uvxx/Async.h"

PCMachine::PCMachine(Manager *manager,
                     ClipboardBase *clipboard,
                     const std::shared_ptr<uvxx::Loop> &uvLoop,
                     QDBusConnection service,
                     uint32_t id,
                     const std::filesystem::path &dataDir,
                     const std::string &ip,
                     uint16_t port,
                     const DeviceInfo &sp)
    : Machine(manager, clipboard, uvLoop, service, id, dataDir, ip, port, sp) {
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
