#include "PCMachine.h"

#include "utils/message_helper.h"
#include "protocol/message.pb.h"

PCMachine::PCMachine(Manager *manager,
                     ClipboardBase *clipboard,
                     QDBusConnection service,
                     uint32_t id,
                     const std::filesystem::path &dataDir,
                     const std::string &ip,
                     uint16_t port,
                     const DeviceInfo &sp)
    : Machine(manager, clipboard, service, id, dataDir, ip, port, sp) {
}

void PCMachine::handleConnected() {
    mountFs("/");
}

void PCMachine::handleDisconnected() {
}

void PCMachine::sendFiles(const QStringList &filePaths) {
    for (const QString &filePath : filePaths) {
        Message msg;
        FsSendFileRequest *send = msg.mutable_fssendfilerequest();
        send->set_path(filePath.toStdString());
        sendMessage(msg);
    }
}

void PCMachine::mountFs(const std::string &path) {
    Message msg;
    auto *request = msg.mutable_fsrequest();
    request->set_path(path);
    sendMessage(msg);
}
