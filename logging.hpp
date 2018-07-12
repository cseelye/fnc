#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"
#include "util/container_util.hpp"

namespace fnc
{

/**
 * @brief Name to give the logger
 */
constexpr auto fnc_log_name = "fnc";

/**
 * @brief Get an instance of the logger, fully configured and ready to use
 * @return the logger
 */
std::shared_ptr<spdlog::logger> get_log();

/**
 * @brief Turn on debug logging so it is displayed on the console
 */
void enable_debug_logging();

void enable_trace_logging();

/**
 * @brief Send a message to the log
 */
template <typename... Args>
void send_to_log(const spdlog::level::level_enum& level, const std::string& filename, const std::string& function, const int& line_number, const std::string& fmt, const Args&... args)
{
    std::string message = fmt::format(fmt, args...);
    get_log()->log(level, "[{}::{}:{}] {}", filename, function, line_number, message);
}
template <typename Arg>
void send_to_log(const spdlog::level::level_enum& level, const std::string& filename, const std::string& function, const int& line_number, const Arg& arg)
{
    get_log()->log(level, "[{}::{}:{}] {}", filename, function, line_number, arg);
}

//
// Logging helper macros that add the context to the log message
//

#define LOG_TRACE(...) \
    send_to_log(spdlog::level::trace, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) \
    send_to_log(spdlog::level::debug, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) \
    send_to_log(spdlog::level::info, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) \
    send_to_log(spdlog::level::warn, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) \
    send_to_log(spdlog::level::err, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) \
    send_to_log(spdlog::level::critical, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

} // end namespace fnc
