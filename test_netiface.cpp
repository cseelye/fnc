#include <boost/algorithm/string/predicate.hpp>
#include <string>
#include <vector>

#include "spdlog/fmt/fmt.h"
#include "catch.hpp"
#include "netiface.hpp"
#include "logging.h"
#include "util/test.hpp"

using namespace fnc;

class SingleDummyNicFixture
{
public:
    std::string iface_name = "test0";
    std::string ipv4_address = "172.16.85.42";
    int ipv4_prefix = 23;
    std::string ipv6_address = "2001:0db8:0:f101::1";
    int ipv6_prefix = 96;

    int iface_index;
    std::string iface_mac;
    mtu_t iface_mtu;

    SingleDummyNicFixture()
    {
        Setup();
    }

    ~SingleDummyNicFixture()
    {
        Teardown();
    }

    void Setup()
    {
        shell_exec("sysctl -w net.ipv6.conf.default.disable_ipv6=0");

        shell_exec(fmt::format("ip link del {} &>/dev/null", iface_name));
        shell_exec(fmt::format("ip link add {} type dummy", iface_name));
        shell_exec(fmt::format("ip link set {} up", iface_name));
        shell_exec(fmt::format("ip addr add {}/{} dev {}", ipv4_address, ipv4_prefix, iface_name));
        shell_exec(fmt::format("ip addr add {}/{} dev {}", ipv6_address, ipv6_prefix, iface_name));

        std::string stdout;
        stdout = shell_exec(fmt::format("cat /sys/class/net/{}/ifindex | tr -d '\n'", iface_name));
        iface_index = std::stoi(stdout);
        iface_mac = shell_exec(fmt::format("cat /sys/class/net/{}/address | tr -d '\n'", iface_name));
        stdout = shell_exec(fmt::format("cat /sys/class/net/{}/mtu | tr -d '\n'", iface_name));
        iface_mtu = std::stoi(stdout);
    }

    void Teardown()
    {
//        shell_exec(fmt::format("ip link del {}", iface_name));
    }
};

TEST_CASE_METHOD(SingleDummyNicFixture, "netiface:Get all ifaces", "[netiface]")
{
    auto ifaces = netiface::get_iface_names();
    REQUIRE(ifaces.size() > 0);
}

TEST_CASE_METHOD(SingleDummyNicFixture, "netiface:Get iface properties", "[netiface]")
{
    netiface nic(iface_name);

    // Name
    REQUIRE(nic.get_name() == iface_name);

    // Index
    REQUIRE(nic.get_index() == iface_index);

    // IP addresses - default
    auto default_ips = nic.get_ip_addresses();
    REQUIRE(default_ips.size() == 3); // v4 + v6 + v6 link local

    // IP addresses - v4
    auto v4_ips = nic.get_ip_addresses(ip_family_type::v4);
    REQUIRE(v4_ips.size() == 1);

    // IP addresses - v6
    auto v6_ips = nic.get_ip_addresses(ip_family_type::v6);
    REQUIRE(v6_ips.size() == 2); // v6 + v6 link local

    // IP addresses - all
    auto all_ips = nic.get_ip_addresses(ip_family_type::all);
    REQUIRE(all_ips.size() == 3); // v4 + v6 + v6 link local

    // MAC address
    REQUIRE(boost::iequals(nic.get_mac_address().to_string(), iface_mac));

    // MTU
    REQUIRE(nic.get_mtu() == iface_mtu);
}

TEST_CASE_METHOD(SingleDummyNicFixture, "netiface:Add IP address", "[netiface]")
{
    netiface nic(iface_name);

    ip_address ip4("2.2.2.2", 18);
    nic.set_ip_address(ip4);
    REQUIRE(contains(nic.get_ip_addresses(), ip4));

    ip_address ip6("2001:0db8:0:f101::2", 64);
    nic.set_ip_address(ip6);
    REQUIRE(contains(nic.get_ip_addresses(), ip6));
}

TEST_CASE_METHOD(SingleDummyNicFixture, "netiface:Set MTU", "[netiface]")
{
    netiface nic(iface_name);
    nic.set_mtu(7777);
    REQUIRE(shell_exec(fmt::format("cat /sys/class/net/{}/mtu | tr -d '\n'", iface_name)) == "7777");
}

TEST_CASE_METHOD(SingleDummyNicFixture, "netiface:Del IP address", "[netiface]")
{
    netiface nic(iface_name);

    nic.del_ip_address(ip_address(ipv4_address));
    REQUIRE(shell_exec(fmt::format("ip addr show {} | grep {} || echo -n false", iface_name, ipv4_address)) == "false");
    nic.del_ip_address(ip_address(ipv4_address));

    nic.del_ip_address(ip_address(ipv6_address, ipv6_prefix));
    REQUIRE(shell_exec(fmt::format("ip addr show {} | grep {} || echo -n false", iface_name, ipv6_address)) == "false");
    nic.del_ip_address(ip_address(ipv6_address));
}
