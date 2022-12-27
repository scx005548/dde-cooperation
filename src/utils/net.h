#ifndef UTILS_NET_H
#define UTILS_NET_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <string>

#include <QTcpSocket>
#include <QDebug>

namespace Net {

inline std::string getHostname(void) {
    char hostName[_SC_HOST_NAME_MAX];
    gethostname(hostName, _SC_HOST_NAME_MAX);
    return hostName;
}

inline void setKeepAlive(QTcpSocket *socket) {
    int enable = 1;
    if (setsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) !=
        0) {
        qWarning("fail to enable SO_KEEPALIVE");
    }

    int idle = 3;
    if (setsockopt(socket->socketDescriptor(),
                   SOL_TCP,
                   TCP_KEEPIDLE,
                   &idle,
                   sizeof(idle)) != 0) {
        qWarning("fail to set TCP_KEEPIDLE");
    }

    int retryIntvl = 1;
    if (setsockopt(socket->socketDescriptor(),
                   SOL_TCP,
                   TCP_KEEPINTVL,
                   &retryIntvl,
                   sizeof(retryIntvl)) != 0) {
        qWarning("fail to set TCP_KEEPINTVL");
    }

    int retryCnt = 3;
    if (setsockopt(socket->socketDescriptor(),
                   SOL_TCP,
                   TCP_KEEPCNT,
                   &retryCnt,
                   sizeof(retryCnt)) != 0) {
        qWarning("fail to set TCP_KEEPCNT");
    }
}

} // namespace Net

#endif // !UTILS_NET_H
