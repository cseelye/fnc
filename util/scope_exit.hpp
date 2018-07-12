#pragma once

#include <unistd.h>

#include "util.hpp"

/**
 * Run arbitrary code when exiting a scope.
 * Based on ideas from https://stackoverflow.com/questions/10270328/the-simplest-and-neatest-c11-scopeguard
 */
template <typename F>
class scope_exit
{
public:
    scope_exit(F f) : f(f) { }

    ~scope_exit() { f(); }

private:
    F f;
};
template <typename F>
scope_exit<F> make_scope_exit(F f)
{
    return scope_exit<F>(f);
}
#define SCOPE_EXIT(code) \
    auto STRING_JOIN(_scope_exit_, __LINE__) = make_scope_exit([=](){ code })


/**
 * RAII wrapper for raw file descriptors
 */
class scoped_filedescriptor
{
public:
    scoped_filedescriptor(int fd) : mFD(fd) { }
    ~scoped_filedescriptor()
    {
        close(mFD);
    }

    int raw()
    {
        return mFD;
    }

private:
    int mFD;
};
