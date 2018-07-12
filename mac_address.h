#pragma once

#include <ostream>
#include <string>

namespace fnc
{

class mac_address
{
public:

    mac_address();
    mac_address(uint8_t* addr);
    mac_address(const std::string& addr);

    std::string to_string() const;
    uint64_t to_uint64();
    void get_bytes(uint8_t* bytes) const;

    friend std::ostream &operator<<(std::ostream&, const mac_address&);

    bool operator==(const mac_address& other);
    bool operator!=(const mac_address& other);

    bool operator==(const std::string& other);
    bool operator!=(const std::string& other);
    bool operator==(const char* other);
    bool operator!=(const char* other);

private:
    uint64_t address_;
};

} // end namespace fnc
