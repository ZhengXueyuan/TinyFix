#pragma once

#include <iostream>
#include <sys/epoll.h>
#include <functional>

namespace TinyFix {

class DefaultEpollerComponents
{
public:
    constexpr static int   EPOLL_MODE = EPOLLIN;
    constexpr static int   NUM_EVENTS = 8;
    constexpr static auto& outStream = std::cout;
    using OutStreamType = std::ostream;
    using EpollerFdCallback = std::function<void( void )>;
};

class EpollerConfigBase
{
public:
    virtual const bool getNonBlock() = 0;
    virtual ~EpollerConfigBase(){};
};

class DefaultEpollerConfig : public EpollerConfigBase
{
public:
    virtual const bool getNonBlock()
    {
        return true;
    }
    virtual ~DefaultEpollerConfig(){};
};

} // namespace TinyFix
