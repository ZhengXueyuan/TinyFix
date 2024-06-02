#pragma once

#include <array>
#include <errno.h>
#include <iostream>
#include <string>
#include <thread>
// memset
#include <string.h>
// socket headers
#include <sys/socket.h>
#include <sys/types.h>
// sockaddr_in
#include <netinet/in.h>
// inet_pton
#include <arpa/inet.h>
// fcntl
#include <fcntl.h>
// shutdown and close
#include <unistd.h>
// misc
#include "socket_misc.h"

namespace TinyFix {

enum SocketType
{
    TCPSocketType,
    UDPSocketType,
    MulticastType,
    // UNIXSocketType,
    DUMMYSocketType,
};

template<typename SocketComponentsT>
class SocketBase
{
public:
    constexpr static size_t BUF_SIZE = SocketComponentsT::SOCKET_RECV_BUF_SIZE;
    using OutType = typename SocketComponentsT::OutStreamType;
    constexpr static auto& out = SocketComponentsT::outStream;
    using BufType = std::array<uint8_t, BUF_SIZE>;

    SocketBase( const SocketBase& ) = delete;
    SocketBase& operator=( const SocketBase& ) = delete;

    SocketBase( SocketConfigBase& config )
        : m_config( config )
        , m_fd( -1 )
    {
    }
    ~SocketBase()
    {
        release();
    }

    bool bind()
    {
        if( ::bind( m_fd, (struct sockaddr*)&m_servAddr, sizeof( m_servAddr ) ) == -1 )
        {
            out << "bind socket error: " << ::strerror( errno ) << ". (errno: " << errno << ")"
                << std::endl;
            return false;
        }
        return true;
    }

    bool setNonBlock()
    {
        if( m_config.getNonBlock() )
        {
            int flags = ::fcntl( m_fd, F_GETFL, 0 );
            if( ::fcntl( m_fd, F_SETFL, flags | O_NONBLOCK ) < 0 )
            {
                ::close( m_fd );
                m_fd = -1;
                out << "Set NONBLOCK failed" << std::endl;
                return false;
            }
        }
        return true;
    }

    bool setSockaddrIn()
    {
        ::memset( &m_servAddr, 0, sizeof( m_servAddr ) );
        m_servAddr.sin_family = AF_INET;
        m_servAddr.sin_port = ::htons( m_config.getPort() );
        if( m_config.getIP().empty() )
        {
            m_servAddr.sin_addr.s_addr = ::htonl( INADDR_ANY );
        }
        else
        {
            if( ::inet_pton( AF_INET, m_config.getIP().c_str(), &m_servAddr.sin_addr ) <= 0 )
            {
                out << "inet_pton error for " << m_config.getIP().c_str() << std::endl;
                return false;
            }
        }
        return true;
    }

    // to avoid TIME_WAIT status, we can use this flag to use the address
    // immediately after close a socket of that address.
    bool setReUseAddr()
    {
        int reuse = 1;
        if( ::setsockopt( m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) ) < 0 )
        {
            ::close( m_fd );
            out << "set udp reuseable failed" << errno << std::endl;
            return false;
        }
        return true;
    }

    /*
     * 主要分四种情况：
     *   1.l_onoff为0。则马上关闭socket（graceful），closesocket马上返回。并尽量在后台将内核发送缓冲区的数据发出去。这种情况正常四次挥手。
     *   2.l_onoff非0，l_linger为0。closesocket马上返回（abortive），连接重置，发送RST到对端，并且丢弃内核发送缓冲区中的数据。这种情况非正常四次挥手，不会time_wait。
     *   3.l_onoff非0，l_linger非0。这种情况又分为阻塞和非阻塞。
     *   对于阻塞socket，则延迟l_linger秒关闭socket，直到发完数据或超时。超时则连接重置，发送RST到对端（abortive），发完则是正常关闭（graceful）。
     *   对于非阻塞socket，如果closesocket不能立即完成，则马上返回错误WSAEWOULDBLOCK。
     * 一般我们都是用情况1，跟不设置LINGER一样。这种情况在服务端的缺点是可能有大量处于time_wait的socket，占用服务器资源。
     * 而对于非法连接，或者客户端已经主动关闭连接，或者服务端想要重启，我们可以使用情况2，强制关闭连接，这样可以避免time_wait。
     */
    bool setLinger()
    {
        struct linger lingerStruct;
        lingerStruct.l_onoff = 1;
        lingerStruct.l_linger = 0;
        return ::setsockopt(
                   m_fd, SOL_SOCKET, SO_LINGER, (char*)&lingerStruct, sizeof( lingerStruct ) ) == 0;
    }

    bool listen()
    {
        if( ::listen( m_fd, m_config.getBacklog() ) == -1 )
        {
            return false;
        }
        return true;
    }

    bool connect()
    {
        if( ::connect( m_fd, (struct sockaddr*)&m_servAddr, sizeof( m_servAddr ) ) < 0 )
        {
            out << "connect error: " << ::strerror( errno ) << ". (errno: " << errno << ")"
                << std::endl;
            return false;
        }
        return true;
    }

    bool release()
    {
        if( m_fd != -1 )
        {
            ::shutdown( m_fd, SHUT_RDWR );
            ::close( m_fd );
            m_fd = -1;
        }
        return true;
    }

    ssize_t send( const char* data, size_t size )
    {
        ssize_t res = ::send( m_fd, data, size, 0 );
        if( res < 0 )
        {
            out << "send error: " << ::strerror( errno ) << ". (errno: " << errno << ")"
                << std::endl;
        }
        return res;
    }

    ssize_t recv()
    {
        ssize_t res = ::recv( m_fd, &m_buf[0], BUF_SIZE, 0 );
        if( res < 0 ||
            ( res == 0 && ( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) ) )
        {
            out << "recv error: " << ::strerror( errno ) << ". (errno: " << errno << ")"
                << std::endl;
        }
        return res;
    }

    int getFd()
    {
        return m_fd;
    }

    void setFd( int fd )
    {
        m_fd = fd;
    }

    BufType& buf()
    {
        return m_buf;
    }

    const SocketConfigBase& getConfig()
    {
        return m_config;
    }

    struct sockaddr_in& socketAddr()
    {
        return m_servAddr;
    }

protected:
    const SocketConfigBase& m_config;
    int                     m_fd;
    struct sockaddr_in      m_servAddr;
    BufType                 m_buf;

private:
};

template<typename SocketComponentsT>
class TCPSocket : public SocketBase<SocketComponentsT>
{
public:
    using OutType = typename SocketComponentsT::OutStreamType;
    constexpr static auto& out = SocketComponentsT::outStream;

    TCPSocket( const TCPSocket& ) = delete;
    TCPSocket& operator=( const TCPSocket& ) = delete;

    TCPSocket( SocketConfigBase& config )
        : SocketBase<SocketComponentsT>( config )
    {
    }
    ~TCPSocket()
    {
    }

    bool create()
    {
        SocketBase<SocketComponentsT>::setFd( ::socket( AF_INET, SOCK_STREAM, 0 ) );
        if( SocketBase<SocketComponentsT>::getFd() == -1 )
        {
            out << "create socket fd failed, error: " << ::strerror( errno )
                << ". error no: " << errno << std::endl;
            return false;
        }
        return true;
    }

    int accept()
    {
        int res = ::accept( SocketBase<SocketComponentsT>::getFd(), (struct sockaddr*)NULL, NULL );
        if( res == -1 )
        {
            out << "accept failed, error: " << ::strerror( errno ) << ". error no: " << errno
                << std::endl;
        }
        return res;
    }

    int accept( struct sockaddr_in& clnt_addr )
    {
        static socklen_t addrSize = sizeof( sockaddr_in );

        int res = ::accept( SocketBase<SocketComponentsT>::getFd(),
                            reinterpret_cast<struct sockaddr*>( &clnt_addr ),
                            &addrSize );
        if( res == -1 )
        {
            out << "accept failed, error: " << ::strerror( errno ) << ". error no: " << errno
                << std::endl;
        }
        return res;
    }
};

template<typename SocketComponentsT>
class UDPSocket : public SocketBase<SocketComponentsT>
{
public:
    constexpr static size_t BUF_SIZE = SocketComponentsT::SOCKET_RECV_BUF_SIZE;
    using OutType = typename SocketComponentsT::OutStreamType;
    constexpr static auto& out = SocketComponentsT::outStream;

    UDPSocket( const UDPSocket& ) = delete;
    UDPSocket& operator=( const UDPSocket& ) = delete;

    UDPSocket( SocketConfigBase& config )
        : SocketBase<SocketComponentsT>( config )
    {
    }
    ~UDPSocket()
    {
    }

    bool create()
    {
        SocketBase<SocketComponentsT>::setFd( ::socket( AF_INET, SOCK_DGRAM, 0 ) );
        if( SocketBase<SocketComponentsT>::getFd() == -1 )
        {
            out << "create socket fd failed, error: " << ::strerror( errno )
                << ". error no: " << errno << std::endl;
            return false;
        }
        if( SocketBase<SocketComponentsT>::getConfig().getReUseAddr() )
        {
            if( !SocketBase<SocketComponentsT>::setReUseAddr() )
            {
                out << "set reuse address failed: " << ::strerror( errno )
                    << ". error no: " << std::endl;
                return false;
            }
        }
        return true;
    }

    ssize_t recvFrom( struct sockaddr_in& recvAddr = SocketBase<SocketComponentsT>::socketAddr() )
    {
        static socklen_t len = sizeof( recvAddr );
        ssize_t          res = ::recvfrom( SocketBase<SocketComponentsT>::getFd(),
                                  SocketBase<SocketComponentsT>::buf().data(),
                                  BUF_SIZE,
                                  0,
                                  (struct sockaddr*)&recvAddr,
                                  &len );
        if( res < 0 ||
            ( res == 0 && ( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) ) )
        {
            out << "accept failed, error: " << ::strerror( errno ) << ". error no: " << errno
                << std::endl;
        }
        else if( res > 0 )
        {
            SocketBase<SocketComponentsT>::buf()[res] = 0;
        }
        return res;
    }

    ssize_t sendTo( struct sockaddr_in& sendAddr, char* data, size_t size )
    {
        static socklen_t len = sizeof( sendAddr );

        ssize_t res = ::sendto( SocketBase<SocketComponentsT>::getFd(),
                                data,
                                size,
                                0,
                                (struct sockaddr*)&sendAddr,
                                len );
        return res;
    }
};

template<typename SocketComponentsT>
class MulticastSocket : public SocketBase<SocketComponentsT>
{
public:
    constexpr static size_t BUF_SIZE = SocketComponentsT::SOCKET_RECV_BUF_SIZE;
    using OutType = typename SocketComponentsT::OutStreamType;
    constexpr static auto& out = SocketComponentsT::outStream;

    MulticastSocket( MulticastSocket& ) = delete;
    MulticastSocket& operator=( const MulticastSocket& ) = delete;

    MulticastSocket( MulticastSocketConfigBase& config )
        : SocketBase<SocketComponentsT>( config )
        , m_config( config )
    {
    }
    ~MulticastSocket()
    {
    }

    bool create()
    {
        SocketBase<SocketComponentsT>::setFd( ::socket( AF_INET, SOCK_DGRAM, 0 ) );
        if( SocketBase<SocketComponentsT>::getFd() < 0 )
        {
            out << "create socket failed: " << ::strerror( errno ) << ". error no: " << errno
                << std::endl;
            return false;
        }
        if( !SocketBase<SocketComponentsT>::setReUseAddr() )
        {
            out << "set reuse address failed: " << ::strerror( errno )
                << ". error no: " << std::endl;
            return false;
        }
        return true;
    }

    bool joinMulticastGroup()
    {
        if( m_config.getSourceIP().empty() )
        {
            out << "IGMP v2" << std::endl;
            struct ip_mreq mreq;
            mreq.imr_multiaddr.s_addr = inet_addr( m_config.getIP().c_str() );
            mreq.imr_interface.s_addr = inet_addr( m_config.getInterfaceIP().c_str() );
            int err = ::setsockopt( SocketBase<SocketComponentsT>::getFd(),
                                    IPPROTO_IP,
                                    IP_ADD_MEMBERSHIP,
                                    &mreq,
                                    sizeof( mreq ) );
            if( err < 0 )
            {
                ::close( SocketBase<SocketComponentsT>::getFd() );
                out << __LINE__ << " ERR:setsockopt(): IP_ADD_MEMBERSHIP error: " << errno
                    << std::endl;
                return false;
            }
        }
        else
        {
            out << "IGMP v3" << std::endl;
            struct ip_mreq_source GroupInfo;
            ::memset( &GroupInfo, 0x00, sizeof( GroupInfo ) );
            GroupInfo.imr_multiaddr.s_addr = ::inet_addr( m_config.getIP().c_str() );
            GroupInfo.imr_sourceaddr.s_addr = ::inet_addr( m_config.getSourceIP().c_str() );
            GroupInfo.imr_interface.s_addr = ::inet_addr( m_config.getInterfaceIP().c_str() );
            if( ::setsockopt( SocketBase<SocketComponentsT>::getFd(),
                              IPPROTO_IP,
                              IP_ADD_SOURCE_MEMBERSHIP,
                              (char*)&GroupInfo,
                              sizeof( GroupInfo ) ) != 0 )
            {
                ::close( SocketBase<SocketComponentsT>::getFd() );
                out << " setsockopt fail IP_ADD_SOURCE_MEMBERSHIP, " << m_config.getIP() << " "
                    << m_config.getSourceIP() << " " << m_config.getInterfaceIP() << " "
                    << std::endl;
                return false;
            }
        }
        out << SocketBase<SocketComponentsT>::getFd()
            << ":Increment Add Membership:" << m_config.getIP().c_str()
            << " port:" << m_config.getPort() << std::endl;
        return true;
    }

    ssize_t recvFrom( struct sockaddr_in& recvAddr = SocketBase<SocketComponentsT>::socketAddr() )
    {
        static socklen_t len = sizeof( recvAddr );

        ssize_t res = ::recvfrom( SocketBase<SocketComponentsT>::getFd(),
                                  SocketBase<SocketComponentsT>::buf().data(),
                                  BUF_SIZE,
                                  0,
                                  (struct sockaddr*)&recvAddr,
                                  &len );
        if( res < 0 ||
            ( res == 0 && ( errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR ) ) )
        {
            out << "accept failed, error: " << ::strerror( errno ) << ". error no: " << errno
                << std::endl;
        }
        else if( res > 0 )
        {
            SocketBase<SocketComponentsT>::buf()[res] = 0;
        }
        return res;
    }

private:
    const MulticastSocketConfigBase& m_config;
};

} // namespace TinyFix
