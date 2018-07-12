#pragma once


#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <random>

#include "ip_address.hpp"
#include "logging.h"

namespace fnc
{

// This is fairly limited and dangerous, only use for testing
// https://stackoverflow.com/a/10702464
std::string shell_exec(const char* cmd);
std::string shell_exec(const std::string& cmd);

int random_int(int min, int max);

ip_address random_ipv4();
ip_address random_ipv6();

}
