#pragma once

#include <atomic>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>

namespace fnc
{

struct in6_ifreq
{
    struct in6_addr addr;
    uint32_t        prefixlen;
    unsigned int    ifindex;
};

struct nl_req_t
{
    struct nlmsghdr hdr;
    union
    {
        struct rtgenmsg rtgen;
        struct ifaddrmsg ifaddr;
        struct ifinfomsg ifinfo;
    };
    char attrbuf[1024];
};


struct nl_msg
{
    struct msghdr rtnl_msg;
    struct iovec io;
    nl_req_t req;
    struct sockaddr_nl kernel_sa;
};

class netlink
{
public:
    const static time_t NL_SOCKET_TIMEOUT = 2;

    netlink();
    virtual ~netlink();

    nl_msg init_message(uint16_t nlmsg_type, uint16_t nlmsg_flags);

    void send_message_sync(const nl_msg& msg, std::function<void(struct nlmsghdr*)> callback);
    void send_message_sync(const nl_msg& msg);

    void send_message_async(const nl_msg& msg);
    void handle_response_async(std::function<void(struct nlmsghdr*)> callback = nullptr);

private:
    static std::atomic_int session_;
    pid_t pid_;
    int nl_sock_;
    uint32_t seq_;
};

} // end namespace fnc
