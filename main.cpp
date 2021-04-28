#include <vector>
#include <CLI11.hpp>
#include "exceptions.hpp"
#include "netiface.hpp"
#include "logging.hpp"
#include "netlink.hpp"

#include "mac_address.hpp"

using namespace fnc;

int main(int argc, char** argv)
{
    CLI::App app{ "FreeNetConfig" };
    int debug = 0;
    std::string iface_name;

    app.add_option("-i, --interface", iface_name, "Interface to configure")->required();
    app.add_flag("-d, --debug", debug, "Display debug messages");
    CLI11_PARSE(app, argc, argv);

    if (debug > 0)
    {
        enable_debug_logging();
    }
    if (debug > 1)
    {
        enable_trace_logging();
    }

    try
    {

        std::vector<std::string> ifaces = netiface::get_iface_names();
        LOG_INFO("ifaces=[{}]", join(ifaces));

        auto nic = netiface(iface_name);

        if (nic.get_name() != iface_name)
            LOG_ERROR("Incorrect name");


        LOG_INFO("name={}", nic.get_name());
        LOG_INFO("index={}", nic.get_index());

        nic.del_ip_address("2.2.2.2");
        LOG_INFO("all IPs=[{}]", join(nic.get_ip_addresses()));
        LOG_INFO("v4 IPs=[{}]", join(nic.get_ip_addresses(ip_family_type::v4)));
        LOG_INFO("v6 IPs=[{}]", join(nic.get_ip_addresses(ip_family_type::v6)));

        nic.set_mtu(1500);
        nic.set_ip_address("1.1.1.1");
        LOG_INFO("MTU={}", nic.get_mtu());
        LOG_INFO("MAC={}", nic.get_mac_address());

        nic.set_mtu(8000);
        LOG_INFO("MTU={}", nic.get_mtu());
        nic.set_ip_address("2.2.2.2");

        try
        {
            nic.set_ip_address("2001:db8:8714:3a90::12");
        }
        catch (const permission_denied&)
        {
            LOG_DEBUG("Permission denied - IPv6 is probably disabled");
        }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << '\n';
        }

    }
    catch (const network_exception& ex)
    {
        LOG_ERROR(ex.what());
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Unexpected exception: {}", ex.what());
    }

    return 0;
}
