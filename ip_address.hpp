#pragma once

#include <boost/asio/ip/address.hpp>
#include <ostream>
#include <string>
#include <sys/socket.h>

namespace fnc
{

enum ip_address_scope
{
    global = 0,
    host = 254
};

class ip_address : public boost::asio::ip::address
{
public:
    static const ip_address ANY;
    static const ip_address LOOPBACK;
    static const ip_address ANYv6;
    static const ip_address LOOPBACKv6;
    static const ip_address UNSPECIFIED;

    using boost::asio::ip::address::address;

#ifdef _VSCODE
    ip_address();
#endif

    ip_address(const std::string& address, int prefix = -1);
    ip_address(const char* address, int prefix = -1);
    ip_address(const sockaddr_in* addr, int prefix = -1);

    friend bool operator==(const ip_address& lhs, const ip_address& rhs);
    friend bool operator!=(const ip_address& lhs, const ip_address& rhs);

    std::string to_string(bool include_prefix = true) const;
    friend std::ostream& operator<<(std::ostream&, const ip_address&);

    uint8_t operator[] (int idx) const;

    sockaddr_in to_sockaddr_in();
    sockaddr_in6 to_sockaddr_in6();
    sockaddr_storage to_sockaddr();

    ip_address without_prefix() const;

    int get_prefix() const;
    ip_address_scope get_scope() const;

private:
    int prefix_ = -1;
    ip_address_scope scope_ = ip_address_scope::global;
};

} // end namespace fnc
