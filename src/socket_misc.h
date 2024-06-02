#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>

namespace TinyFix {

class DefaultSocketComponents
{
public:
    using OutStreamType = std::ostream;
    constexpr static auto& outStream = std::cout;
    // 4MB of receive buffer is quite enough
    constexpr static size_t SOCKET_RECV_BUF_SIZE = 1024 * 1024 * 4;
};

class SocketConfigBase
{
public:
    using IpType = std::string;

    virtual const int     getPort() const = 0;
    virtual const IpType& getIP() const = 0;
    virtual const bool    getNoDelay() const = 0;
    virtual const bool    getNonBlock() const = 0;
    virtual const bool    getReUseAddr() const = 0;
    virtual const int     getBacklog() const = 0;
};

class MulticastSocketConfigBase : public SocketConfigBase
{
public:
    virtual const std::string& getInterfaceIP() const = 0;
    virtual const IpType&      getSourceIP() const = 0;

    virtual ~MulticastSocketConfigBase()
    {
    }
};

class DefaultSocketConfig : public SocketConfigBase
{
public:
    DefaultSocketConfig( int    port = 13898,
                         IpType ip = "127.0.0.1",
                         bool   noDelay = false,
                         bool   nonBlock = false,
                         bool   reUseAddr = false,
                         int    backlog = 4 )
        : m_port( port )
        , m_ip( ip )
        , m_noDelay( noDelay )
        , m_nonBlock( nonBlock )
        , m_reUseAddr( reUseAddr )
        , m_backlog( backlog )
    {
    }

    ~DefaultSocketConfig()
    {
    }

    virtual const int getPort() const
    {
        return m_port;
    }
    virtual const IpType& getIP() const
    {
        return m_ip;
    }
    virtual const bool getNoDelay() const
    {
        return m_noDelay;
    }
    virtual const bool getNonBlock() const
    {
        return m_nonBlock;
    }
    virtual const bool getReUseAddr() const
    {
        return m_reUseAddr;
    }
    virtual const int getBacklog() const
    {
        return m_backlog;
    }

private:
    const int    m_port;
    const IpType m_ip;
    const bool   m_noDelay;
    const bool   m_nonBlock;
    const bool   m_reUseAddr;
    const int    m_backlog;
};

class DefaultMulticastSocketConfig : public MulticastSocketConfigBase
{
public:
    DefaultMulticastSocketConfig( IpType interfaceIP,
                                  int    port = 4000,
                                  IpType ip = "224.0.0.100",
                                  IpType sourceIP = "",
                                  bool   noDelay = true,
                                  bool   nonBlock = true,
                                  bool   reUseAddr = true,
                                  int    backlog = 4 )
        : m_interfaceIP( interfaceIP )
        , m_sourceIP( sourceIP )
        , m_port( port )
        , m_ip( ip )
        , m_noDelay( noDelay )
        , m_nonBlock( nonBlock )
        , m_reUseAddr( reUseAddr )
        , m_backlog( backlog )
    {
        std::cout << m_interfaceIP << std::endl;
        std::cout << getInterfaceIP() << std::endl;
    }

    ~DefaultMulticastSocketConfig()
    {
    }

    virtual const int getPort() const
    {
        return m_port;
    }

    virtual const IpType& getIP() const
    {
        return m_ip;
    }

    virtual const bool getNoDelay() const
    {
        return m_noDelay;
    }

    virtual const bool getNonBlock() const
    {
        return m_nonBlock;
    }

    virtual const bool getReUseAddr() const
    {
        return m_reUseAddr;
    }

    virtual const int getBacklog() const
    {
        return m_backlog;
    }

    virtual const std::string& getInterfaceIP() const
    {
        return m_interfaceIP;
    }

    virtual const IpType& getSourceIP() const
    {
        return m_sourceIP;
    }

private:
    const IpType m_interfaceIP;
    const IpType m_sourceIP;
    IpType       m_ip;
    int          m_port;
    bool         m_noDelay;
    bool         m_nonBlock;
    bool         m_reUseAddr;
    int          m_backlog;
};

} // namespace TinyFix
