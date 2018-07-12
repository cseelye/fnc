#include <algorithm>
#include <arpa/inet.h>
#include <iomanip>
#include <net/if.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <sstream>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "netiface.hpp"
#include "exceptions.hpp"
#include "logging.h"
#include "netlink.h"
#include "util/container_util.hpp"
#include "util/mac_helper.hpp"
#include "util/scope_exit.hpp"
#include "util/retry_wait.hpp"

namespace fnc
{

std::ostream& operator << (std::ostream& os, const ip_family_type& obj)
{
   os << static_cast<std::underlying_type<ip_family_type>::type>(obj);
   return os;
}

std::vector<std::string> netiface::get_iface_names()
{
    std::vector<std::string> iface_names;

    netlink nl;
    nl_msg message = nl.init_message(RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP);
    nl.send_message_sync(message, [&](struct nlmsghdr* msg_ptr) {
        if (msg_ptr->nlmsg_type != RTM_NEWLINK)
        {
            LOG_WARN("Recieved unknown message type {}", msg_ptr->nlmsg_type);
            return;
        }

        struct ifinfomsg* iface_info = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(msg_ptr));
        int len = msg_ptr->nlmsg_len - NLMSG_LENGTH(sizeof(*iface_info));

        for (struct rtattr* attribute = IFLA_RTA(iface_info); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
        {
            switch (attribute->rta_type)
            {
            case IFLA_IFNAME:
                iface_names.push_back(reinterpret_cast<char*>(RTA_DATA(attribute)));
                break;
            default:
                break;
            }
        }
    });
    return iface_names;
}

netiface::netiface(const std::string& name)
{
    std::vector<std::string> all_ifaces = get_iface_names();
    if (!contains(all_ifaces, name))
    {
        LOG_DEBUG("all_ifaces=[{}]", join(all_ifaces));
        THROW_NETEX("Specified interface does not exist name={}", name);
    }
    name_ = name;
}

std::string netiface::get_name()
{
    return name_;
}

iface_idx_t netiface::get_index()
{
    int iface_index = -1;

    netlink nl;
    nl_msg message = nl.init_message(RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP);

    nl.send_message_sync(message, [&](struct nlmsghdr* msg_ptr) {
        if (msg_ptr->nlmsg_type != RTM_NEWLINK)
        {
            LOG_WARN("Recieved unknown message type {}", msg_ptr->nlmsg_type);
            return;
        }

        struct ifinfomsg* iface_info = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(msg_ptr));
        int len = msg_ptr->nlmsg_len - NLMSG_LENGTH(sizeof(*iface_info));

        for (struct rtattr* attribute = IFLA_RTA(iface_info); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
        {
            switch (attribute->rta_type)
            {
            case IFLA_IFNAME:
                if (name_ == std::string(reinterpret_cast<char*>(RTA_DATA(attribute))))
                {
                    iface_index = iface_info->ifi_index;
                }
                return;
            default:
                break;
            }

            if (iface_index > 0)
            {
                break;
            }
        }
    });

    return iface_index;
}

mtu_t netiface::get_mtu()
{
    mtu_t iface_mtu;
    iface_idx_t iface_idx = get_index();

    netlink nl;
    nl_msg message = nl.init_message(RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP);
    nl.send_message_sync(message, [&](struct nlmsghdr* msg_ptr) {
        if (msg_ptr->nlmsg_type != RTM_NEWLINK)
        {
            LOG_WARN("Recieved unknown message type {}", msg_ptr->nlmsg_type);
            return;
        }

        struct ifinfomsg* iface_info = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(msg_ptr));
        if (iface_info->ifi_index != iface_idx)
        {
            return;
        }

        int len = msg_ptr->nlmsg_len - NLMSG_LENGTH(sizeof(*iface_info));
        for (struct rtattr* attribute = IFLA_RTA(iface_info); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
        {
            switch (attribute->rta_type)
            {
            case IFLA_MTU:
                iface_mtu = *reinterpret_cast<mtu_t*>(RTA_DATA(attribute));
                return;
            default:
                break;
            }
        }
    });
    return iface_mtu;
}

void netiface::set_mtu(mtu_t mtu)
{
    LOG_DEBUG("Setting MTU={} iface={}", mtu, name_);
    iface_idx_t iface_idx = get_index();

    netlink nl;
    nl_msg message = nl.init_message(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);
    message.req.ifinfo.ifi_index = iface_idx;
    message.req.ifinfo.ifi_change = 0xFFFFFFFF;

    struct rtattr *attr = (struct rtattr *)(((char *) &message.req) + NLMSG_ALIGN(message.req.hdr.nlmsg_len));
    attr->rta_type = IFLA_MTU;
    attr->rta_len = RTA_LENGTH(sizeof(mtu));
    message.req.hdr.nlmsg_len = NLMSG_ALIGN(message.req.hdr.nlmsg_len) + RTA_LENGTH(sizeof(mtu));
    memcpy(RTA_DATA(attr), &mtu, sizeof(mtu));

    // Send the message and wait for an ACK
    nl.send_message_sync(message);

    // Verify MTU was updated
    WaitFor(std::chrono::milliseconds(500), fmt::format("Failed to set MTU={} on iface={}", mtu, name_), [&]{
        return get_mtu() == mtu;
    });
}

mac_address netiface::get_mac_address()
{
    mac_address mac_addr;
    iface_idx_t iface_idx = get_index();

    netlink nl;
    nl_msg message = nl.init_message(RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP);
    nl.send_message_sync(message, [&](struct nlmsghdr* msg_ptr) {
        if (msg_ptr->nlmsg_type != RTM_NEWLINK)
        {
            LOG_WARN("Recieved unknown message type {}", msg_ptr->nlmsg_type);
            return;
        }

        struct ifinfomsg* iface_info = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(msg_ptr));
        if (iface_info->ifi_index != iface_idx)
        {
            return;
        }

        int len = msg_ptr->nlmsg_len - NLMSG_LENGTH(sizeof(*iface_info));
        for (struct rtattr* attribute = IFLA_RTA(iface_info); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
        {
            switch (attribute->rta_type)
            {
                case IFLA_ADDRESS:
                    mac_addr = mac_address(reinterpret_cast<uint8_t*>(RTA_DATA(attribute)));
                    return;
                default:
                    break;
            }
        }
    });
    return mac_addr;
}

std::vector<ip_address> netiface::get_ip_addresses(const ip_family_type& ip_family)
{
    return get_ip_addresses_impl(ip_family, true);
}

std::vector<ip_address> netiface::get_ip_addresses_impl(const ip_family_type& ip_family, bool include_prefix)
{
    bool get_v4 = (ip_family & ip_family_type::v4) == ip_family_type::v4;
    bool get_v6 = (ip_family & ip_family_type::v6) == ip_family_type::v6;

    iface_idx_t iface_idx = get_index();
    netlink nl;
    nl_msg message = nl.init_message(RTM_GETADDR, NLM_F_REQUEST | NLM_F_MATCH);
    message.req.ifaddr.ifa_index = iface_idx;

    std::vector<ip_address> addresses;
    nl.send_message_sync(message, [&](struct nlmsghdr* msg_ptr) {
        if (msg_ptr->nlmsg_type != RTM_NEWADDR)
        {
            LOG_INFO("Recieved unknown message type {}", msg_ptr->nlmsg_type);
            return;
        }

        struct ifaddrmsg* addr_info = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(msg_ptr));
        if (addr_info->ifa_index != (uint32_t)iface_idx)
        {
            return;
        }

        int len = msg_ptr->nlmsg_len - NLMSG_LENGTH(sizeof(*addr_info));
        char ipaddr_str[INET6_ADDRSTRLEN];
        for (struct rtattr* attribute = IFA_RTA(addr_info); RTA_OK(attribute, len); attribute = RTA_NEXT(attribute, len))
        {
            switch (attribute->rta_type)
            {
                case IFA_ADDRESS:
                    if (addr_info->ifa_family == AF_INET && !get_v4)
                    {
                        return;
                    }
                    if (addr_info->ifa_family == AF_INET6 && !get_v6)
                    {
                        return;
                    }
                    inet_ntop(addr_info->ifa_family, RTA_DATA(attribute), ipaddr_str, sizeof(ipaddr_str));
                    if (include_prefix)
                    {
                        addresses.emplace_back(ipaddr_str, (int)addr_info->ifa_prefixlen);
                    }
                    else
                    {
                        addresses.emplace_back(ipaddr_str);
                    }
                    break;
                /*
                case IFA_LABEL:
                    LOG_DEBUG("    label {}", (char*)RTA_DATA(attribute));
                    break;
                case IFA_LOCAL:
                    inet_ntop(addr_info->ifa_family, RTA_DATA(attribute), ipaddr_str, sizeof(ipaddr_str));
                    LOG_DEBUG("    local {}", ipaddr_str);
                    break;
                case IFA_BROADCAST:
                    inet_ntop(addr_info->ifa_family, RTA_DATA(attribute), ipaddr_str, sizeof(ipaddr_str));
                    LOG_DEBUG("    broadcast {}", ipaddr_str);
                    break;
                case IFA_ANYCAST:
                    inet_ntop(addr_info->ifa_family, RTA_DATA(attribute), ipaddr_str, sizeof(ipaddr_str));
                    LOG_DEBUG("    anycast {}", ipaddr_str);
                    break;
                case IFA_MULTICAST:
                    inet_ntop(addr_info->ifa_family, RTA_DATA(attribute), ipaddr_str, sizeof(ipaddr_str));
                    LOG_DEBUG("    multicast {}", ipaddr_str);
                    break;
                case IFA_FLAGS:
                    LOG_DEBUG("    flags {}", *(uint32_t*)RTA_DATA(attribute));
                */
                default:
                    break;
            }
        }
    });
    //LOG_DEBUG("Found ip addresses [{}]", join(addresses));
    return addresses;
}

void netiface::set_ip_address(ip_address address)
{
    if (address.get_prefix() < 0)
    {
        THROW_NETEX("Invalid or unspecified prefix length for address={}", address);
    }

    LOG_DEBUG("Adding address={} to iface={}", address, name_);
    if (contains(get_ip_addresses(), address))
    {
        LOG_DEBUG("Address={} already deleted from iface={}", address, name_);
        return;
    }

    iface_idx_t iface_idx = get_index();
    netlink nl;
    nl_msg message = nl.init_message(RTM_NEWADDR, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    message.req.ifaddr.ifa_index = iface_idx;
    message.req.ifaddr.ifa_prefixlen = address.get_prefix();
    message.req.ifaddr.ifa_scope = address.get_scope();
    message.req.ifaddr.ifa_family = address.is_v4() ? AF_INET : AF_INET6;

    struct rtattr *attr = (struct rtattr *)(((char *) &message.req) + NLMSG_ALIGN(message.req.hdr.nlmsg_len));
    attr->rta_type = IFA_LOCAL;

    if (address.is_v4())
    {
        struct sockaddr_in addr = address.to_sockaddr_in();
        attr->rta_len = RTA_LENGTH(sizeof(addr.sin_addr));
        memcpy(RTA_DATA(attr), &addr.sin_addr, sizeof(addr.sin_addr));
        message.req.hdr.nlmsg_len = NLMSG_ALIGN(message.req.hdr.nlmsg_len) + RTA_LENGTH(sizeof(addr.sin_addr));
    }
    else if (address.is_v6())
    {
        struct sockaddr_in6 addr = address.to_sockaddr_in6();
        attr->rta_len = RTA_LENGTH(sizeof(addr.sin6_addr));
        memcpy(RTA_DATA(attr), &addr.sin6_addr, sizeof(addr.sin6_addr));
        message.req.hdr.nlmsg_len = NLMSG_ALIGN(message.req.hdr.nlmsg_len) + RTA_LENGTH(sizeof(addr.sin6_addr));
    }

    // Send the message and wait for an ACK
    nl.send_message_sync(message);

    // Verify IP is on the interface
    WaitFor(std::chrono::milliseconds(500), fmt::format("Failed to add IP address={} to iface={}", address, name_), [&]{
        return contains(get_ip_addresses(), address);
    });
}

void netiface::del_ip_address(ip_address address)
{
    LOG_DEBUG("Deleting address={} iface={}", address, name_);
    auto current_ips = get_ip_addresses();
    bool already_deleted;
    if (address.is_v4())
    {
        // Compare without prefix, because prefix does not matter (we delete with prefix 32)
        already_deleted = !contains(get_ip_addresses(), address, [&address](const auto& item) { return item.without_prefix() == address.without_prefix(); });
    }
    else
    {
        already_deleted = !contains(get_ip_addresses(), address);
    }
    if (already_deleted)
    {
        LOG_DEBUG("Address={} already deleted from iface={}", address, name_);
        return;
    }

    iface_idx_t iface_idx = get_index();
    netlink nl;
    nl_msg message = nl.init_message(RTM_DELADDR, NLM_F_REQUEST | NLM_F_ACK);
    message.req.ifaddr.ifa_index = iface_idx;
    message.req.ifaddr.ifa_prefixlen = address.is_v4() ? 32 :  address.get_prefix();
    message.req.ifaddr.ifa_scope = address.get_scope();
    message.req.ifaddr.ifa_family = address.is_v4() ? AF_INET : AF_INET6;

    struct rtattr *attr = (struct rtattr *)(((char *) &message.req) + NLMSG_ALIGN(message.req.hdr.nlmsg_len));
    attr->rta_type = IFA_LOCAL;

    if (address.is_v4())
    {
        struct sockaddr_in addr = address.to_sockaddr_in();
        attr->rta_len = RTA_LENGTH(sizeof(addr.sin_addr));
        memcpy(RTA_DATA(attr), &addr.sin_addr, sizeof(addr.sin_addr));
        message.req.hdr.nlmsg_len = NLMSG_ALIGN(message.req.hdr.nlmsg_len) + RTA_LENGTH(sizeof(addr.sin_addr));
    }
    else if (address.is_v6())
    {
        struct sockaddr_in6 addr = address.to_sockaddr_in6();
        attr->rta_len = RTA_LENGTH(sizeof(addr.sin6_addr));
        memcpy(RTA_DATA(attr), &addr.sin6_addr, sizeof(addr.sin6_addr));
        message.req.hdr.nlmsg_len = NLMSG_ALIGN(message.req.hdr.nlmsg_len) + RTA_LENGTH(sizeof(addr.sin6_addr));
    }

    // Send the message and wait for an ACK
    nl.send_message_sync(message);

    // Verify IP is gone from the interface
    WaitFor(std::chrono::milliseconds(500), fmt::format("Failed to delete IP address={} from iface={}", address, name_), [&]{
        return !contains(get_ip_addresses_impl(ip_family_type::all, false), address.without_prefix());
    });
}

} // end namespace fnc
