#ifndef UVXX_ADDR_H
#define UVXX_ADDR_H

#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>

namespace uvxx {

class IPv4Addr;
class IPv6Addr;

class Addr : public std::enable_shared_from_this<Addr> {
public:
    static std::shared_ptr<Addr> create();
    static std::shared_ptr<Addr> create(const sockaddr *sa);

    sockaddr *get() { return reinterpret_cast<sockaddr *>(&addr_); }
    const sockaddr *get() const { return reinterpret_cast<const sockaddr *>(&addr_); }
    sa_family_t family() const { return addr_.ss_family; }

    std::string toString() const;

    std::shared_ptr<IPv4Addr> ipv4();
    std::shared_ptr<IPv6Addr> ipv6();

protected:
    Addr() = default;
    sockaddr_storage addr_;
};

class IPv4Addr : public Addr {
public:
    static std::shared_ptr<IPv4Addr> create(const std::string &ip, uint16_t port);

    sockaddr_in *get() { return reinterpret_cast<sockaddr_in *>(&addr_); }
    const sockaddr_in *get() const { return reinterpret_cast<const sockaddr_in *>(&addr_); }

    uint16_t port() const;
    void setPort(uint16_t port);

    std::string ip() const;

private:
    using Addr::Addr;
};

class IPv6Addr : public Addr {
public:
    static std::shared_ptr<IPv6Addr> create(const std::string &ip, uint16_t port);

    sockaddr_in6 *get() { return reinterpret_cast<sockaddr_in6 *>(&addr_); }
    const sockaddr_in6 *get() const { return reinterpret_cast<const sockaddr_in6 *>(&addr_); }

    uint16_t port() const;
    void setPort(uint16_t port);

    std::string ip() const;

private:
    using Addr::Addr;
};

} // namespace uvxx

#endif // !UVXX_ADDR_H
