#pragma once

#include <utility>
#include <array>
#include <map>
#include <functional>

#include <sys/epoll.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "epoller_misc.h"

namespace TinyFix {

template<typename EpollerComponents>
class Epoller
{
public:
    constexpr static int EPOLL_MODE = EpollerComponents::EPOLL_MODE;
    constexpr static int NUM_EVENTS = EpollerComponents::NUM_EVENTS;
    using EpollerFdCallback = typename EpollerComponents::EpollerFdCallback;
    using OutType = typename EpollerComponents::OutStreamType;
    constexpr static auto& out = EpollerComponents::outStream;

    using FdCallbacks = std::map<const int, const EpollerFdCallback>;
    using EventArray = std::array<struct epoll_event, NUM_EVENTS>;

    enum EpollFdOperation : int
    {
        EPOLL_ADD = EPOLL_CTL_ADD,
        EPOLL_DEL = EPOLL_CTL_DEL,
        EPOLL_MOD = EPOLL_CTL_MOD,
    };

    Epoller( Epoller& ) = delete;
    Epoller& operator=( Epoller& ) = delete;

    Epoller( EpollerConfigBase& config )
        : m_epollFd( ::epoll_create( 4096 ) )
        , m_epollWaitMilliSecs( config.getNonBlock() ? 0 : -1 )
        , m_countReadyFds( 0 )
    {
    }

    virtual ~Epoller()
    {
        ::close( m_epollFd );
    }

    void setWaitSec( int waitSec )
    {
        m_epollWaitMilliSecs = waitSec * 1000;
    }

    int getEpollerFd() const
    {
        return m_epollFd;
    }

    bool addEvent( const int fd, const EpollerFdCallback& callback )
    {
        if( fd < 0 )
        {
            out << "fd error: " << fd << std::endl;
        }
        static EpollFdOperation operation = EpollFdOperation::EPOLL_ADD;

        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLL_MODE;
        if( ::epoll_ctl( m_epollFd, static_cast<int>( operation ), fd, &ev ) == -1 )
        {
            out << "Epoller Add fd failed. fd: " << fd << std::endl;
            return false;
        }
        out << "Epoller Add fd success. fd: " << fd << std::endl;
        m_callbacks.insert( std::pair<const int, const EpollerFdCallback>( fd, callback ) );
        // m_callbacks[fd] = callback;
        return true;
    }

    bool removeEvent( int fd )
    {
        if( fd < 0 )
        {
            out << "fd error: " << fd << std::endl;
        }
        static EpollFdOperation operation = EPOLL_CTL_DEL;

        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLL_MODE;
        if( ::epoll_ctl( m_epollFd, static_cast<int>( operation ), fd, &ev ) == -1 )
        {
            out << "Epoller remove fd failed. fd: " << fd << std::endl;
            return false;
        }
        m_callbacks.erase( fd );
        return true;
    }

    bool poll()
    {
        m_countReadyFds = ::epoll_wait( m_epollFd, &m_events[0], NUM_EVENTS, m_epollWaitMilliSecs );
        // std::cout << "m_countReadyFds " << m_countReadyFds << std::endl;

        if( m_countReadyFds <= 0 )
        {
            if( m_countReadyFds == 0 || errno == EINTR )
            {
                return true;
            }
            out << "Exit from epoller: " << errno << " : " << ::strerror( errno ) << std::endl;
            return false;
        }
        else
        {
            //  std::cout << "m_countReadyFds " << m_countReadyFds << std::endl;

            for( int i = 0; i < m_countReadyFds; i++ )
            {
                // if( m_events[i].events & EPOLLOUT )
                // {
                //     out << m_events[i].data.fd << " : "
                //         << "EPOLLOUT" << std::endl;
                // }
                // if( m_events[i].events & EPOLLIN )
                // {
                //     out << m_events[i].data.fd << " : "
                //         << "EPOLLIN" << std::endl;
                // }
                m_callbacks[m_events[i].data.fd]();
                return true;
            }
        }
        return false;
    }

private:
    int m_epollFd;
    int m_epollWaitMilliSecs;
    int m_countReadyFds;

    EventArray  m_events;
    FdCallbacks m_callbacks;
};

} // namespace TinyFix
