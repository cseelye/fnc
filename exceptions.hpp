#pragma once

#include <string>
#include <exception>
#include <sstream>
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"

namespace fnc
{

/**
 * @brief Base exception class
 *
 */
class fnc_exception : public std::exception
{
public:
    const std::string file;         ///< File the exception was thrown from
    const std::string function;     ///< Function the exception was thrown from
    const int line;                 ///< Line number the exceptionw was thrown from
    const std::string message;      ///< Exception message

    /**
     * @brief Constructor
     *
     * @param message   Exception message
     */
    fnc_exception(const std::string& message)
        : file(""),
          function(""),
          line(0),
          message(message)
    { }

    /**
     * @brief Constructor
     *
     * @param filename  File the exception was thrown from
     * @param funcname  Function the exception was thrown from
     * @param line      Line number the exceptionw was thrown from
     * @param message   Exception message
     */
    fnc_exception(const std::string& filename, const std::string& funcname, const int& line, const std::string& message)
        : file(filename),
          function(funcname),
          line(line),
          message(message)
    { }

    virtual const char* what() const noexcept
    {
        std::string fullmsg = "";
        if (!file.empty() && !function.empty() && line > 0)
        {
            fullmsg = fmt::format("[{}::{}:{}] ", file, function, line);
        }
        fullmsg.append(message);
        return fullmsg.c_str();
    }
};

/**
 * @brief Exception thrown when there is a network related error
 */
class network_exception : public fnc_exception
{
    using fnc_exception::fnc_exception;
};

/**
 * @brief Exception thrown when there is a timeout
 */
class timeout_exception : public fnc_exception
{
    using fnc_exception::fnc_exception;
};

class illegal_argument : public fnc_exception
{
    using fnc_exception::fnc_exception;
};

class permission_denied : fnc_exception
{
    using fnc_exception::fnc_exception;
};

/**
 * @brief Throw an exception and automatically include the file, function, line number thrown from
 */
#define THROWEX(exception_name_, ...) \
    throw exception_name_(__FILE__, __FUNCTION__, __LINE__, fmt::format(__VA_ARGS__))

/**
 * @brief Throw a network exception and automatically include the file, function, line number thrown from
 */
#define THROW_NETEX(...) \
    throw network_exception(__FILE__, __FUNCTION__, __LINE__, fmt::format(__VA_ARGS__))

/**
 * @brief Throw a timeout exception and automatically include the file, function, line number thrown from
 */
#define THROW_TIMEOUT(...) \
    throw timeout_exception(__FILE__, __FUNCTION__, __LINE__, fmt::format(__VA_ARGS__))

/**
 * @brief Throw a permission denied exception and automatically include the file, function, line number thrown from
 */
#define THROW_DENIED(...) \
    throw permission_denied(__FILE__, __FUNCTION__, __LINE__, fmt::format(__VA_ARGS__))

} // end namespace fnc
