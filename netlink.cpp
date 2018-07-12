
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>

#include "netlink.h"
#include "exceptions.hpp"
#include "logging.h"
#include "util/mac_helper.hpp"

namespace fnc
{

std::atomic_int netlink::session_(0);

netlink::netlink()
    : pid_(0),
      nl_sock_(0),
      seq_(0)
{
    pid_ = syscall(SYS_gettid) + session_++;

    nl_sock_ = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    struct timeval timeout;
    timeout.tv_sec = NL_SOCKET_TIMEOUT;
    timeout.tv_usec = 0;

    if (setsockopt(nl_sock_, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        THROW_NETEX("Failed to set timeout on netlink socket");
    }

    struct sockaddr_nl local_sa;
    memset(&local_sa, 0, sizeof(local_sa));
    local_sa.nl_family = AF_NETLINK;
    local_sa.nl_pid = pid_;
    local_sa.nl_groups = 0;
    LOG_TRACE("Binding netlink socket with pid={}", pid_);
    if (bind(nl_sock_, (struct sockaddr *) &local_sa, sizeof(local_sa)) != 0)
    {
        THROW_NETEX("Failed to bind netlink socket: {}", strerror(errno));
    }
}

netlink::~netlink()
{
    LOG_TRACE("Closing netlink socket with pid={}", pid_);
    close(nl_sock_);
}

nl_msg netlink::init_message(uint16_t nlmsg_type, uint16_t nlmsg_flags)
{
    struct nl_msg message;
    memset(&message.kernel_sa, 0, sizeof(message.kernel_sa));
    memset(&message.rtnl_msg, 0, sizeof(message.rtnl_msg));
    memset(&message.req, 0, sizeof(message.req));

    message.kernel_sa.nl_family = AF_NETLINK;

    message.req.hdr.nlmsg_seq = ++seq_;
    message.req.hdr.nlmsg_pid = pid_;

    message.req.hdr.nlmsg_flags = nlmsg_flags;
    message.req.hdr.nlmsg_type = nlmsg_type;

    switch (nlmsg_type)
    {
        case RTM_NEWLINK:
        case RTM_DELLINK:
            message.req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
            break;
        case RTM_GETLINK:
            message.req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
            break;

        case RTM_NEWADDR:
        case RTM_DELADDR:
        case RTM_GETADDR:
            message.req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
            break;

        default:
            THROW_NETEX("Unknown netlink message type={}", nlmsg_type);
    }

    message.io.iov_base = &message.req;
    message.io.iov_len = message.req.hdr.nlmsg_len;
    message.rtnl_msg.msg_iov = &message.io;
    message.rtnl_msg.msg_iovlen = 1;
    message.rtnl_msg.msg_name = &message.kernel_sa;
    message.rtnl_msg.msg_namelen = sizeof(message.kernel_sa);

    return message;
}

void netlink::send_message_sync(const nl_msg &msg, std::function<void (struct nlmsghdr*)> callback)
{
    send_message_async(msg);
    handle_response_async(callback);
}

void netlink::send_message_sync(const nl_msg &msg)
{
    send_message_async(msg);
    handle_response_async();
}

void netlink::send_message_async(const nl_msg &msg)
{
    LOG_TRACE("Sending message for pid={} seq={}", msg.req.hdr.nlmsg_pid, msg.req.hdr.nlmsg_seq);
    int rc = send(nl_sock_, &msg.req, msg.req.hdr.nlmsg_len, 0);
//    int rc = sendmsg(nl_sock_, (struct msghdr *) &msg.rtnl_msg, 0);
    if (rc < 0)
    {
        THROW_NETEX("Error sending netlink message: {}", strerror(errno));
    }
}

void netlink::handle_response_async(std::function<void (struct nlmsghdr*)> callback)
{
    int BUFFER_LEN = 16384;

    struct sockaddr_nl kernel_sa;
    memset(&kernel_sa, 0, sizeof(kernel_sa));
    kernel_sa.nl_family = AF_NETLINK;

    bool done = false;
    char reply[BUFFER_LEN];
    while (!done)
    {
        int len;
        struct msghdr rtnl_reply;
        struct iovec io_reply;
        memset(&io_reply, 0, sizeof(io_reply));
        memset(&rtnl_reply, 0, sizeof(rtnl_reply));

        io_reply.iov_base = reply;
        io_reply.iov_len = BUFFER_LEN;
        rtnl_reply.msg_iov = &io_reply;
        rtnl_reply.msg_iovlen = 1;
        rtnl_reply.msg_name = &kernel_sa;
        rtnl_reply.msg_namelen = sizeof(kernel_sa);

        len = recvmsg(nl_sock_, &rtnl_reply, 0);
        if (len < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                THROW_NETEX("Timeout waiting to receive netlink message");
            }
            THROW_NETEX("Error receiving netlink message: {}", strerror(errno));
        }

        for (struct nlmsghdr *msg_ptr = (struct nlmsghdr *)reply; NLMSG_OK(msg_ptr, len); msg_ptr = NLMSG_NEXT(msg_ptr, len))
        {
            switch(msg_ptr->nlmsg_type)
            {
                case NLMSG_DONE:
                    LOG_TRACE("DONE recieved for pid={} seq={}", msg_ptr->nlmsg_pid, msg_ptr->nlmsg_seq);
                    done = true;
                    break;

                case NLMSG_ERROR:
                {
                    struct nlmsgerr *err = reinterpret_cast<struct nlmsgerr*>(NLMSG_DATA(msg_ptr));
                    if (err->error == 0)
                    {
                        // NLMSG_ERROR with the code set to 0 is a reply for a message that requested NLM_F_ACK
                        LOG_TRACE("ACK recieved for pid={} seq={}", msg_ptr->nlmsg_pid, msg_ptr->nlmsg_seq);
                        return;
                    }
                    THROW_NETEX("netlink error {}", strerror(-(err->error)));
                }

                default:
                    if (callback)
                    {
                        callback(msg_ptr);
                    }
                    break;
            }
        }
    }
}

} // end namespace fnc
