#pragma once

#include <net/if.h>
#include <string>
#include <vector>
#include "ip_address.hpp"
#include "mac_address.hpp"
#include "netlink.hpp"
#include "util/mac_helper.hpp"
#include "util/bitmask_operators.hpp"

namespace fnc
{
using iface_idx_t = int;
using mtu_t = uint32_t;

enum class ip_family_type
{
    v4  = 0x1,
    v6  = 0x2,
    all = 0x3
};
ENABLE_BITMASK_OPERATORS(ip_family_type);
std::ostream& operator << (std::ostream& os, const ip_family_type& obj);

class netiface
{
public:

    /**
     * @brief Get a list of all network interfaces on this system
     *
     * @return list of names
     */
    static std::vector<std::string> get_iface_names();

    /**
     * Constructor
     * @param name  the name of the interface
     */
    netiface(const std::string& name);

    /**
     * @brief Get the name of this interface
     *
     * @return name of the interface
     */
    std::string get_name();

    /**
     * @brief Get the index of this interface
     *
     * @return index of the interface
     */
    iface_idx_t get_index();

    /**
     * @brief Get the MTU of this interface
     *
     * @return MTU of the interface
     */
    mtu_t get_mtu();

    /**
     * @brief Set the MTU of this interface
     *
     * @param mtu   MTU to set
     */
    void set_mtu(mtu_t mtu);

    /**
     * @brief Get the MAC address of this interface
     *
     * @return the interface MAC address
     */
    mac_address get_mac_address();

    /**
     * @brief Get the ip addresses of this interface
     *
     * Both IPv4 and IP46 are returned by default. This can be controlled with the ip_family argument
     * The ip_address also contains the prefix length (i.e. subnet mask)
     *
     * @param ip_family     The family of addresses to return - ipv4, ipv6, or both
     * @return a vector of IP addresses that are assigned to this interface
     */
    std::vector<ip_address> get_ip_addresses(const ip_family_type& ip_family = ip_family_type::all);

    /**
     * @brief Add an IP address to this interface
     *
     * The IP address can be v4 or v6, and this function is idempotent - if the address is already assigned
     * to the interface, no action is taken
     *
     * @param address   The IP address to add
     */
    void set_ip_address(ip_address address);

    /**
     * @brief Remove an IP address from the interface
     *
     * The IP address can be v4 or v6, and this function is idempotent - if the address is not assigned to
     * the interface, no action is taken
     *
     * @param address   The IP address to remove
     */
    void del_ip_address(ip_address address);

private:
    std::string name_;

    std::vector<ip_address> get_ip_addresses_impl(const ip_family_type& ip_family = ip_family_type::all, bool include_prefix = true);

};

} // end namespace fnc
