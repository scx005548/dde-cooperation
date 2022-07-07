#include "Addr.h"

#include <string>

#include <cstring>

#include <arpa/inet.h>

using namespace uvxx;

std::shared_ptr<Addr> Addr::create() {
    return std::shared_ptr<Addr>(new Addr);
}

std::shared_ptr<Addr> Addr::create(const sockaddr *sa) {
    auto a = std::shared_ptr<Addr>(new Addr);
    memcpy(a->get(), sa, sizeof(*sa));

    return a;
}

std::string Addr::toString() const {
    switch (get()->sa_family) {
    case AF_INET: {
        auto *ipv4 = reinterpret_cast<const IPv4Addr *>(this);
        return ipv4->ip() + ':' + std::to_string(ipv4->port());
    }
    case AF_INET6: {
        auto *ipv6 = reinterpret_cast<const IPv6Addr *>(this);
        return std::string{'['} + ipv6->ip() + ']' + ':' + std::to_string(ipv6->port());
    }
    }

    return "";
}

std::shared_ptr<IPv4Addr> Addr::ipv4() {
    if (get()->sa_family != AF_INET) {
        return nullptr;
    }

    return std::static_pointer_cast<IPv4Addr>(shared_from_this());
}

std::shared_ptr<IPv6Addr> Addr::ipv6() {
    if (get()->sa_family != AF_INET6) {
        return nullptr;
    }

    return std::static_pointer_cast<IPv6Addr>(shared_from_this());
}

std::shared_ptr<IPv4Addr> IPv4Addr::create(const std::string &ip, uint16_t port) {
    auto a = std::shared_ptr<IPv4Addr>(new IPv4Addr);

    a->get()->sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &(a->get()->sin_addr));
    a->get()->sin_port = htons(port);
    return a;
}

uint16_t IPv4Addr::port() const {
    return ntohs(get()->sin_port);
}

void IPv4Addr::setPort(uint16_t port) {
    get()->sin_port = htons(port);
}

std::string IPv4Addr::ip() const {
    char buff[INET_ADDRSTRLEN];

    return inet_ntop(AF_INET, &(get()->sin_addr), buff, INET_ADDRSTRLEN);
}

std::shared_ptr<IPv6Addr> IPv6Addr::create(const std::string &ip, uint16_t port) {
    auto a = std::shared_ptr<IPv6Addr>(new IPv6Addr);

    a->get()->sin6_family = AF_INET6;
    inet_pton(AF_INET6, ip.c_str(), &(a->get()->sin6_addr));
    a->get()->sin6_port = htons(port);
    return a;
}

uint16_t IPv6Addr::port() const {
    return ntohs(get()->sin6_port);
}

void IPv6Addr::setPort(uint16_t port) {
    get()->sin6_port = htons(port);
}

std::string IPv6Addr::ip() const {
    char buff[INET6_ADDRSTRLEN];

    return inet_ntop(AF_INET6, &(get()->sin6_addr), buff, INET6_ADDRSTRLEN);
}
