//#include <boost/asio/ip/address.hpp>
#include <array>
#include "test.hpp"

namespace fnc
{

std::string shell_exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

std::string shell_exec(const std::string& cmd)
{
    return shell_exec(cmd.c_str());
}

int random_int(int min, int max)
{
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(min, max);
    return distr(eng);
}

ip_address random_ipv4()
{
    return ip_address(fmt::format("{}.{}.{}.{}",
                                  random_int(1, 255),
                                  random_int(1, 255),
                                  random_int(1, 255),
                                  random_int(1, 255)));
}

ip_address random_ipv6()
{
    std::random_device rd;
    std::mt19937_64 eng(rd());
    uint64_t upper = eng();
    uint64_t lower = eng();

    std::array<unsigned char, 16> addr_bytes;
    for (size_t idx=0; idx<addr_bytes.size(); ++idx)
    {
        if (idx < 8)
            addr_bytes[idx] = (upper >> (8 * (16-idx-1))) & 0xFF;
        else
            addr_bytes[idx] = (lower >> (8 * (16-idx-1))) & 0xFF;
    }
    return ip_address(boost::asio::ip::address_v6(addr_bytes));
}

} // end namespace fnc
