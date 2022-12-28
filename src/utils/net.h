#ifndef UTILS_NET_H
#define UTILS_NET_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>

namespace Net {

inline std::string getHostname(void) {
    char hostName[_SC_HOST_NAME_MAX];
    gethostname(hostName, _SC_HOST_NAME_MAX);
    return hostName;
}

} // namespace Net

#endif // !UTILS_NET_H
