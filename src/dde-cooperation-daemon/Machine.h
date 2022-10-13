#ifndef DDE_COOPERATION_DAEMON_DEVICE_H
#define DDE_COOPERATION_DAEMON_DEVICE_H

#include <filesystem>

#include <glibmm.h>

#include "common.h"

#include "dbus/dbus.h"

#include "protocol/message.pb.h"

namespace uvxx {
class Loop;
class Timer;
class Async;
class TCP;
class UDP;
class Buffer;
} // namespace uvxx

class Manager;
class ClipboardBase;
class Request;
class InputEmittorWrapper;
class FuseServer;
class FuseClient;
class ConfirmDialogWrapper;

class Machine : public std::enable_shared_from_this<Machine> {
    friend Manager;

public:
    Machine(Manager *manager,
            ClipboardBase *clipboard,
            const std::shared_ptr<uvxx::Loop> &uvLoop,
            Glib::RefPtr<DBus::Service> service,
            uint32_t id,
            const std::filesystem::path &dataDir,
            const Glib::ustring &ip,
            uint16_t port,
            const DeviceInfo &sp);
    ~Machine();

    Glib::ustring path() const { return m_path; }
    const Glib::ustring &ip() const { return m_ip; };

    void receivedPing();
    void onPair(const std::shared_ptr<uvxx::TCP> &sock);
    void onInputGrabberEvent(uint8_t deviceType, unsigned int type, unsigned int code, int value);
    void onClipboardTargetsChanged(const std::vector<std::string> &targets);

    void flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept;
    void readTarget(const std::string &target);

protected:
    void pair(const Glib::VariantContainerBase &args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void disconnect(const Glib::VariantContainerBase &args,
                    const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void sendFile(const Glib::VariantContainerBase &args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void requestCooperate(const Glib::VariantContainerBase &args,
                          const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void stopCooperation(const Glib::VariantContainerBase &args,
                         const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;
    void mountFs(const Glib::VariantContainerBase &args,
                 const Glib::RefPtr<Gio::DBus::MethodInvocation> &invocation) noexcept;

    void getIP(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPort(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getUUID(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getName(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getPaired(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getOS(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCompositor(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getCooperating(Glib::VariantBase &property, const Glib::ustring &propertyName) const;
    void getDirection(Glib::VariantBase &property, const Glib::ustring &propertyName) const;

private:
    Manager *m_manager;
    ClipboardBase *m_clipboard;
    const std::filesystem::path m_dataDir;
    const std::filesystem::path m_mountpoint;

    const Glib::ustring m_path;

    Glib::RefPtr<DBus::Service> m_service;
    Glib::RefPtr<DBus::Object> m_object;
    Glib::RefPtr<DBus::Interface> m_interface;

    Glib::RefPtr<DBus::Method> m_methodPair;
    Glib::RefPtr<DBus::Method> m_methodDisconnect;
    Glib::RefPtr<DBus::Method> m_methodSendFile;
    Glib::RefPtr<DBus::Method> m_methodRequestCooperate;
    Glib::RefPtr<DBus::Method> m_methodStopCooperation;
    Glib::RefPtr<DBus::Method> m_methodMountFs;

    Glib::ustring m_ip;
    Glib::RefPtr<DBus::Property> m_propertyIP;

    uint16_t m_port;
    Glib::RefPtr<DBus::Property> m_propertyPort;

    Glib::ustring m_uuid;
    Glib::RefPtr<DBus::Property> m_propertyUUID;

    Glib::ustring m_name;
    Glib::RefPtr<DBus::Property> m_propertyName;

    bool m_paired;
    Glib::RefPtr<DBus::Property> m_propertyPaired;

    DeviceOS m_os;
    Glib::RefPtr<DBus::Property> m_propertyOS;

    Compositor m_compositor;
    Glib::RefPtr<DBus::Property> m_propertyCompositor;

    bool m_deviceSharing;
    Glib::RefPtr<DBus::Property> m_propertyCooperating;

    uint16_t m_direction;
    Glib::RefPtr<DBus::Property> m_propertyDirection;

    std::shared_ptr<uvxx::Loop> m_uvLoop;
    std::shared_ptr<uvxx::Timer> m_pingTimer;
    std::shared_ptr<uvxx::Timer> m_offlineTimer;
    std::shared_ptr<uvxx::Async> m_async;
    std::shared_ptr<uvxx::TCP> m_conn;
    std::unique_ptr<ConfirmDialogWrapper> m_confirmDialog;

    std::unordered_map<InputDeviceType, std::unique_ptr<InputEmittorWrapper>> m_inputEmittors;

    std::unique_ptr<FuseServer> m_fuseServer;
    std::unique_ptr<FuseClient> m_fuseClient;

    bool m_mounted;

    void ping();
    void onOffline();

    void initConnection();
    void mountFs(const std::string &path);

    void handleDisconnected();
    void dispatcher(uvxx::Buffer &buff) noexcept;
    void handlePairResponse(const PairResponse &resp);
    void handleDeviceSharingStartRequest();
    void handleDeviceSharingStartResponse(const DeviceSharingStartResponse &resp);
    void handleDeviceSharingStopRequest();
    void handleInputEventRequest(const InputEventRequest &req);
    void handleFlowRequest(const FlowRequest &req);
    void handleFsRequest(const FsRequest &req);
    void handleFsResponse(const FsResponse &resp);
    void handleFsSendFileRequest(const FsSendFileRequest &req);
    void handleClipboardNotify(const ClipboardNotify &notify);
    void handleClipboardGetContentRequest(const ClipboardGetContentRequest &req);
    void handleClipboardGetContentResponse(const ClipboardGetContentResponse &resp);

    void handleAcceptSendFile(bool accepted,
                              const std::map<Glib::ustring, Glib::VariantBase> &hint,
                              uint32_t serial);
    void handleAcceptFilesystem(bool accepted,
                                const std::map<Glib::ustring, Glib::VariantBase> &hint,
                                uint32_t serial);

    void stopDeviceSharingAux();
    void receivedUserConfirm(uvxx::Buffer &buff);
};

#endif // !DDE_COOPERATION_DAEMON_DEVICE_H
