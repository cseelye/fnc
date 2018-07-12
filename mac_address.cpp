
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "exceptions.hpp"
#include "logging.hpp"
#include "mac_address.hpp"

namespace fnc
{

std::ostream &operator<<(std::ostream& os, const mac_address& obj)
{
    os << obj.to_string();
    return os;
}

mac_address::mac_address()
    : address_(0)
{ }

mac_address::mac_address(uint8_t* addr)
{
    address_ = 0;
    int shift = 40;
    for (int i=0; i < IFHWADDRLEN; ++i)
    {
        address_ += (uint64_t(addr[i]) << shift);
        shift -= 8;
    }
}

mac_address::mac_address(const std::string& addr)
{
    std::string mac = boost::erase_all_copy(addr, ":");
    if (mac.size() != 12 || !boost::all(mac, boost::algorithm::is_xdigit()))
    {
        THROW_NETEX("Invalid MAC address={}", addr);
    }

    address_ = std::strtoul(mac.c_str(), NULL, 16);
}

std::string mac_address::to_string() const
{
    uint8_t bytes[IFHWADDRLEN];
    get_bytes(bytes);

    std::ostringstream ss;
    ss << std::setfill('0') << std::hex << std::uppercase;
    for (int i = 0; i < IFHWADDRLEN; ++i)
    {
        ss << std::setw(2) << (int)bytes[i];
        if (i < IFHWADDRLEN - 1)
            ss << ":";
    }
    return ss.str();
}

uint64_t mac_address::to_uint64()
{
    return address_;
}

void mac_address::get_bytes(uint8_t* bytes) const
{
    for (int i = 0; i < IFHWADDRLEN; ++i)
    {
        bytes[i] = ((address_ >> (8 * (IFHWADDRLEN - i - 1))) & 0xFF);
    }
}

bool mac_address::operator==(const mac_address& other)
{
    return address_ == other.address_;
}

bool mac_address::operator!=(const mac_address& other)
{
    return ! operator!=(other);
}

bool mac_address::operator==(const std::string& other)
{
    return to_string() == other;
}

bool mac_address::operator!=(const std::string& other)
{
    return ! operator!=(other);
}

bool mac_address::operator==(const char* other)
{
    return operator==(std::string(other));
}
bool mac_address::operator!=(const char* other)
{
    return operator!=(std::string(other));
}




} // end namespace fnc
