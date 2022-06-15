#ifndef UTILS_NET_H
#define UTILS_NET_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>

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
