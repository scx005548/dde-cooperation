#ifndef COOPERATION_H
#define COOPERATION_H

#include <glibmm.h>
#include <giomm.h>
#include <arpa/inet.h>

#include <dbus.h>

#include <pair.pb.h>

#include <log.hpp>
#include <net.hpp>
#include <message.hpp>

#include <FileSender.h>
#include <FileReceiver.h>

#define SCAN_KEY "UOS-COOPERATION"

class Cooperation
{
public:
    Cooperation() noexcept;

protected:
    // DBus method handlers
    void scan(const Glib::VariantContainerBase& args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) noexcept;

    void pair(const Glib::VariantContainerBase& args,
              const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) noexcept;

    void sendFile(const Glib::VariantContainerBase& args,
                  const Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation) noexcept;

    // DBus property handlers
    void getDevices(Glib::VariantBase& property,
                    const Glib::ustring& propertyName) const noexcept;

    void getPairedDevice(Glib::VariantBase& property,
                         const Glib::ustring& propertyName) const noexcept;

private:
    Glib::RefPtr<DBus::Service>     m_service;
    Glib::RefPtr<DBus::Object>      m_object;
    Glib::RefPtr<DBus::Interface>   m_interface;

    // DBus methods
    Glib::RefPtr<DBus::Method> m_methodScan;
    Glib::RefPtr<DBus::Method> m_methodPair;
    Glib::RefPtr<DBus::Method> m_methodSendFile;

    // DBus properties
    Glib::RefPtr<DBus::Property> m_propertyDevices;
    std::vector<Glib::ustring> m_devices;

    Glib::RefPtr<DBus::Property> m_propertyPairedDevice;
    Glib::ustring m_pairedDevice;

    Glib::RefPtr<Gio::Socket> m_socketScan;
    Glib::RefPtr<Gio::Socket> m_socketListenScan;
    Glib::RefPtr<Gio::Socket> m_socketListenPair;

    // 不能自己连接自己，实际上只需要一个socket
    // 为了方便测试，允许自己连接自己，因此创建了两个socket
    Glib::RefPtr<Gio::Socket> m_socketConnect;      // 主动连接
    Glib::RefPtr<Gio::Socket> m_socketConnected;    // 被动连接

    const uint16_t m_scanPort = 51595;
    Glib::RefPtr<Gio::SocketAddress> m_scanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenScanAddr;
    Glib::RefPtr<Gio::SocketAddress> m_listenPairAddr;

    FileSender m_fileSender;
    FileReceiver m_fileReciever;

    bool m_scanRequestHandler(Glib::IOCondition cond) const noexcept;
    bool m_scanResponseHandler(Glib::IOCondition cond) noexcept;

    bool m_pairRequestHandler(Glib::IOCondition cond) noexcept;

    bool m_mainHandler(Glib::IOCondition cond, const Glib::RefPtr<Gio::Socket>& sock) noexcept;
};

#endif // COOPERATION_H