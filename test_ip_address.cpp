#include <sstream>

#include "catch.hpp"
#include "exceptions.hpp"
#include "ip_address.hpp"
#include "logging.h"
#include "util/test.hpp"

using namespace fnc;

TEST_CASE("ip_address:ctor", "[ip_address]")
{
    // Default
    ip_address addr;
    REQUIRE(addr.is_unspecified());
    REQUIRE(addr == ip_address::UNSPECIFIED);

    // Invalid IP
    REQUIRE_THROWS_AS(ip_address("asdf"), illegal_argument);
    REQUIRE_THROWS_AS(ip_address("1.1.1.256"), illegal_argument);

    // IPv4 without prefix
    ip_address addr2("1.2.3.4");
    REQUIRE(!addr2.is_unspecified());
    REQUIRE(!addr2.is_loopback());
    REQUIRE(!addr2.is_multicast());
    REQUIRE(!addr2.is_v6());
    REQUIRE(addr2.is_v4());
    REQUIRE(addr2.get_prefix() == 32);
    REQUIRE(addr2.get_scope() == ip_address_scope::global);

    // Copy construction
    ip_address addr2a = addr2;
    REQUIRE(addr2a == addr2);
    REQUIRE(addr2a.to_string() == addr2.to_string());
    REQUIRE(addr2a.get_prefix() == addr2.get_prefix());
    REQUIRE(addr2a.get_scope() == addr2.get_scope());

    // IPv4 loopback
    ip_address addr3("127.0.0.1");
    REQUIRE(!addr3.is_unspecified());
    REQUIRE(addr3.is_loopback());
    REQUIRE(!addr3.is_multicast());
    REQUIRE(!addr3.is_v6());
    REQUIRE(addr3.is_v4());
    REQUIRE(addr3.get_prefix() == 32);
    REQUIRE(addr3.get_scope() == ip_address_scope::host);

    // IPv6 without prefix
    ip_address addr4("b521:60fe:4001:1e89:a344:f60f:15ea:b230");
    REQUIRE(!addr4.is_unspecified());
    REQUIRE(!addr4.is_loopback());
    REQUIRE(!addr4.is_multicast());
    REQUIRE(addr4.is_v6());
    REQUIRE(!addr4.is_v4());
    REQUIRE(addr4 == ip_address("b521:60fe:4001:1e89:a344:f60f:15ea:b230"));
    REQUIRE(addr4.get_prefix() == 64);
    REQUIRE(addr4.get_scope() == ip_address_scope::global);

    // IPv6 loopback
    ip_address addr5("::1");
    REQUIRE(!addr5.is_unspecified());
    REQUIRE(addr5.is_loopback());
    REQUIRE(!addr5.is_multicast());
    REQUIRE(addr5.is_v6());
    REQUIRE(!addr5.is_v4());
    REQUIRE(addr5.get_prefix() == 64);
    REQUIRE(addr5.get_scope() == ip_address_scope::host);

    // IPv4 with prefix
    ip_address addr6("1.2.3.4", 19);
    REQUIRE(!addr6.is_unspecified());
    REQUIRE(!addr6.is_loopback());
    REQUIRE(!addr6.is_multicast());
    REQUIRE(!addr6.is_v6());
    REQUIRE(addr6.is_v4());
    REQUIRE(addr6.get_prefix() == 19);
    REQUIRE(addr6.get_scope() == ip_address_scope::global);

    // Copy construction
    ip_address addr6a = addr6;
    REQUIRE(addr6a == addr6);
    REQUIRE(addr6a.to_string() == addr6.to_string());
    REQUIRE(addr6a.get_prefix() == addr6.get_prefix());
    REQUIRE(addr6a.get_scope() == addr6.get_scope());
}

TEST_CASE("ip_address:equality", "[ip_address]")
{
    ip_address a("1.2.3.4");
    ip_address b("1.2.3.4");
    REQUIRE(a == b);

    a = ip_address("1.2.3.4", 18);
    b = ip_address("1.2.3.4", 18);
    REQUIRE(a == b);

    a = ip_address("1.2.3.4", 18);
    b = ip_address("1.2.3.4", 24);
    REQUIRE(a != b);

    a = ip_address("1.2.3.4", 8);
    b = ip_address("1.2.3.4");
    REQUIRE(a != b);
}

TEST_CASE("ip-address:stringify", "[ip_address]")
{
    std::ostringstream ss;
    ss << ip_address();
    REQUIRE(ss.str() == "0.0.0.0");

    ss.str("");
    ss << ip_address("4.3.2.1");
    REQUIRE(ss.str() == "4.3.2.1");

    ss.str("");
    ss << ip_address("4.3.2.1", 23);
    REQUIRE(ss.str() == "4.3.2.1/23");

    ss.str("");
    ss << ip_address("3049:df0d:a7d7:76f0:f7a5:818b:f168:e8c6");
    REQUIRE(ss.str() == "3049:df0d:a7d7:76f0:f7a5:818b:f168:e8c6/64");

    ss.str("");
    ss << ip_address("3049:df0d:a7d7:76f0:f7a5:818b:f168:e8c6", 96);
    REQUIRE(ss.str() == "3049:df0d:a7d7:76f0:f7a5:818b:f168:e8c6/96");

    REQUIRE(ip_address().to_string() == "0.0.0.0");
    REQUIRE(ip_address("4.3.2.1").to_string() == "4.3.2.1");
    REQUIRE(ip_address("4.3.2.1", 18).to_string() == "4.3.2.1/18");
    REQUIRE(ip_address("aa61:49e1:fa30:9767:84e4:f26c:d058:4728").to_string() == "aa61:49e1:fa30:9767:84e4:f26c:d058:4728/64");
    REQUIRE(ip_address("aa61:49e1:fa30:9767:84e4:f26c:d058:4728", 96).to_string() == "aa61:49e1:fa30:9767:84e4:f26c:d058:4728/96");
}

TEST_CASE("ip-address:sockaddr", "[ip_address]")
{
    ip_address addr = random_ipv4();
    struct sockaddr_in sa = addr.to_sockaddr_in();
    for (size_t idx=0; idx < sizeof(sa.sin_addr.s_addr); ++idx)
    {
        REQUIRE(((unsigned uint8_t *)(&sa.sin_addr.s_addr))[idx] == addr[idx]);
    }
    REQUIRE(sa.sin_family == AF_INET);
    REQUIRE(sa.sin_port == 0);

    ip_address addr2 = random_ipv6();
    struct sockaddr_in6 sa2 = addr2.to_sockaddr_in6();
    for (size_t idx=0; idx < sizeof(sa2.sin6_addr.s6_addr); ++idx)
    {
        REQUIRE(sa2.sin6_addr.s6_addr[idx] == addr2[idx]);
    }
    REQUIRE(sa2.sin6_family == AF_INET6);
    REQUIRE(sa2.sin6_port == 0);
    REQUIRE(sa2.sin6_scope_id == addr2.get_scope());
}
