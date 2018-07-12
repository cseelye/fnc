#pragma once

#include <chrono>
#include <string>
#include <thread>

#include <exceptions.hpp>

namespace fnc
{

/**
 * @brief Retry a condition until it passes or the timeout expires
 *
 * This will execute the give condition function until it returns true, or until the timout expires.
 * If the timeout expires, a timeout_exception is thrown, with the given timeout_message
 *
 * @example
 * @code
 * // Try sometest for 5 seconds
 * WaitFor(std::chrono::seconds(5), "Timeout trying to do something", [&]{
 *     if (sometest)
 *         return true;
 *     return false;
 * });
 * @endcode
 *
 * @param timeout           How long to retry for
 * @param timeout_message   Message to use in the exception if a timeout occurs
 * @param condition         Function to call to determine if the condition is pass or fail
 * @param wait              How long to wait between each try
 */
template <typename Rep, typename Period, typename Condition>
void WaitFor(const std::chrono::duration<Rep, Period>& timeout,
             const std::string& timeout_message,
             const Condition& condition,
             const std::chrono::duration<Rep, Period>& wait = std::chrono::milliseconds(50))
{
    auto start_time = std::chrono::system_clock::now();
    while(!condition())
    {
        if (std::chrono::system_clock::now() - start_time > timeout)
        {
            THROW_TIMEOUT(timeout_message);
        }
        std::this_thread::sleep_for(wait);
    }
}

/**
 * @brief Retry a condition a given number of times
 *
 * This will execute the given condition function until it retruns true, or until the number of retries is exhausted
 *
 * @example
 * @code
 * // Try sometest 10 times, once every second
 * RetryFor(5, "Failed sometest after 5 tries", [&]{
 *     if (sometest)
 *         return true;
 *     return false;
 * }, std::chrono::seconds(1))
 * @endcode
 *
 * @param retry_count       Number of times to retry
 * @param timeout_message   Message to use in the exception if retries are exhausted
 * @param condition         Function to call to determine if the condition is pass or fail
 * @param wait              How long to wait between each try
 */
template <typename Condition, typename Rep, typename Period>
void RetryFor(int retry_count,
              const std::string& timeout_message,
              const Condition& condition,
              const std::chrono::duration<Rep, Period>& wait = std::chrono::milliseconds(50))
{
    while (!condition())
    {
        retry_count--;
        if (retry_count <= 0)
        {
            THROW_TIMEOUT(timeout_message);
        }
        std::this_thread::sleep_for(wait);
    }

}

} // end namespace fnc
