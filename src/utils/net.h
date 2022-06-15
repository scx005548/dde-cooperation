#ifndef UTILS_NET_H
#define UTILS_NET_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>

#include <NetworkManager.h>
#include <giomm.h>

namespace Net {

inline std::string getHostname(void) {
    char hostName[_SC_HOST_NAME_MAX];
    gethostname(hostName, _SC_HOST_NAME_MAX);
    return hostName;
}

inline std::string getIpAddress(void) {
    char hostName[_SC_HOST_NAME_MAX];
    gethostname(hostName, _SC_HOST_NAME_MAX);
    struct hostent *hostEntry = gethostbyname(hostName);

    if (hostEntry != nullptr) return inet_ntoa(*(struct in_addr *)(hostEntry->h_addr_list[0]));

    return "";
}

inline guint getIpPrefix(const std::string &addr) {
    NMClient *client = nm_client_new(nullptr, nullptr);
    const GPtrArray *devices = nm_client_get_all_devices(client);

    for (guint i = 0; i < devices->len; i++) {
        NMDevice *device = reinterpret_cast<NMDevice *>(devices->pdata[i]);
        std::string iface = nm_device_get_iface(device);
        if (iface == "lo") continue;

        NMIPConfig *config = nm_device_get_ip4_config(device);
        if (config == nullptr) continue;

        GPtrArray *addresses = nm_ip_config_get_addresses(config);
        for (guint j = 0; j < addresses->len; j++) {
            NMIPAddress *address = reinterpret_cast<NMIPAddress *>(addresses->pdata[j]);
            std::string ip = nm_ip_address_get_address(address);
            if (ip == addr) {
                guint num = nm_ip_address_get_prefix(address);
                g_object_unref(client);
                return num;
            }
        }
    }

    g_object_unref(client);
    return 0;
}

template <int N>
inline constexpr in_addr_t subnetMask(in_addr_t mask = 0) {
    return subnetMask<N - 1>(mask | (1 << (32 - N)));
}

template <>
inline constexpr in_addr_t subnetMask<0>(in_addr_t mask) {
    return mask;
}

inline std::string getBroadcastAddress(const std::string &ip) {
    in_addr_t addr = inet_addr(ip.c_str());
    in_addr_t mask = subnetMask<24>();
    in_addr_t broadcast = addr | htonl(~mask);

    struct in_addr inAddr;
    inAddr.s_addr = broadcast;
    return inet_ntoa(inAddr);
}

inline Glib::RefPtr<Gio::SocketAddress> makeSocketAddress(const std::string &ip, in_port_t port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    return Gio::SocketAddress::create(&addr, sizeof(addr));
}

} // namespace Net

#endif // !UTILS_NET_H
