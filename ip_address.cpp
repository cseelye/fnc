#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <ostream>

#include "exceptions.hpp"
#include "ip_address.hpp"
#include "logging.hpp"

namespace fnc
{

const ip_address ip_address::ANY(boost::asio::ip::address_v4::any());
const ip_address ip_address::LOOPBACK(boost::asio::ip::address_v4::loopback());
const ip_address ip_address::ANYv6(boost::asio::ip::address_v6::any());
const ip_address ip_address::LOOPBACKv6(boost::asio::ip::address_v6::loopback());
const ip_address ip_address::UNSPECIFIED;

#ifdef _VSCODE
ip_address::ip_address()  : boost::asio::ip::address() { }
#endif

ip_address::ip_address(const std::string &address, int prefix)
try : boost::asio::ip::address(boost::asio::ip::address::from_string(address))
{
    prefix_ = prefix;
    if (prefix_ < 0)
    {
        if (is_v4())
            prefix_ = 32;
        else
            prefix_ = 64;
    }

    if (is_loopback())
    {
        scope_ = ip_address_scope::host;
    }
}
catch (boost::system::system_error&)
{
    THROWEX(illegal_argument, "Invalid IP address");
}

ip_address::ip_address(const char *address, int prefix)
    : ip_address(std::string(address), prefix)
{ }

ip_address::ip_address(const sockaddr_in *addr, int prefix)
    : ip_address(inet_ntoa(addr->sin_addr), prefix)
{ }

bool operator==(const ip_address &lhs, const ip_address &rhs)
{
    return dynamic_cast<const boost::asio::ip::address&>(lhs) == dynamic_cast<const boost::asio::ip::address&>(rhs) &&
           lhs.prefix_ == rhs.prefix_ &&
           lhs.scope_ == rhs.scope_;
}

bool operator!=(const ip_address &lhs, const ip_address &rhs)
{
    return ! operator==(lhs, rhs);
}

std::string ip_address::to_string(bool include_prefix) const
{
    std::ostringstream ss;
    ss << dynamic_cast<const boost::asio::ip::address&>(*this);
    if (include_prefix && prefix_ > 0)
    {
        if ((is_v4() && prefix_ != 32) || is_v6())
        {
            ss << "/" << prefix_;
        }
    }
    return ss.str();
}

std::ostream &operator<<(std::ostream& os, const ip_address& obj)
{
    os << obj.to_string();
    return os;
}

uint8_t ip_address::operator[] (int idx) const
{
    if (is_v4())
    {
        return to_v4().to_bytes().at(idx);
    }
    else
    {
        return to_v6().to_bytes().at(idx);
    }
}

sockaddr_in ip_address::to_sockaddr_in()
{
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    int rc = inet_pton(AF_INET, to_string(false).c_str(), &sa.sin_addr);
    if (rc != 1)
    {
        THROW_NETEX("Failed to convert address={} to network format: {}", to_string(), strerror(errno));
    }
    return sa;
}

sockaddr_in6 ip_address::to_sockaddr_in6()
{
    struct sockaddr_in6 sa;
    memset(&sa, 0, sizeof(sockaddr_in6));
    sa.sin6_family = AF_INET6;
    sa.sin6_port = 0;
    sa.sin6_scope_id = scope_;
    int rc = inet_pton(AF_INET6, to_string(false).c_str(), &sa.sin6_addr);
    if (rc != 1)
    {
        THROW_NETEX("Failed to convert address={} to network format: {}", to_string(), strerror(errno));
    }
    return sa;
}

sockaddr_storage ip_address::to_sockaddr()
{
    if (is_v4())
    {
        sockaddr_in sa = to_sockaddr_in();
        return *((sockaddr_storage*)&sa);
    }
    else
    {
        sockaddr_in6 sa = to_sockaddr_in6();
        return *((sockaddr_storage*)&sa);
    }
}

ip_address ip_address::without_prefix() const
{
    return ip_address(to_string(false));
}

int ip_address::get_prefix() const
{
    return prefix_;
}

ip_address_scope ip_address::get_scope() const
{
    return scope_;
}


} // end namespace fnc
