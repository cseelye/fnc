#pragma once

#include <string>
#include <sstream>

/**
 * Test if a container contains an item
 */
template <typename ContainerType, typename ItemType>
bool contains(const ContainerType& container, ItemType item)
{
    return find(container.begin(), container.end(), item) != container.end();
}

template <typename ContainerType, typename ItemType, typename UnaryPredicate>
bool contains(const ContainerType& container, ItemType item, UnaryPredicate pred)
{
    return find_if(container.begin(), container.end(), pred) != container.end();
}

/**
 * Join the items of an iterable container into a string
 */
template<typename InputIt>
std::string join(InputIt begin, InputIt end, const std::string& separator = ",", const std::string& suffix = "")
{
    std::ostringstream ss;

    if (begin != end)
    {
        ss << *begin++;
    }

    while (begin != end)
    {
        ss << separator;
        ss << *begin++;
    }

    ss << suffix;
    return ss.str();
}

template <typename ContainerType>
std::string join(ContainerType container, const std::string& separator = ",", const std::string& suffix = "")
{
    return join(container.begin(), container.end(), separator, suffix);
}
