#ifndef __LINKO_SOCKET_H__
#define __LINKO_SOCKET_H__

#include <memory>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "address.h"
#include "noncopyable.h"

namespace linko {

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX
    };

    // 根据地址创建TCP/UDP套接字
    static Socket::ptr CreateTCP(linko::Address::ptr address);
    static Socket::ptr CreateUDP(linko::Address::ptr address);

    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();
    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();
    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    // 发送超时时间
    int64_t getSendTimeout();
    bool setSendTimeout(int64_t v);

    // 接收超时时间
    int64_t getRecvTimeout();
    bool setRecvTimeout(int64_t v);

    // 获取socketfd信息
    bool getOption(int level, int option, void* result, size_t* len);
    template<class T>
    bool getOption(int level, int option, T& result) {
        size_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    // 设置socketfd信息
    bool setOption(int level, int option, const void* result, size_t len);
    template<class T>
    bool setOption(int level, int option, const T& value) {
        return setOption(level, option, &value, sizeof(T));
    }

    // 接收connect链接
    Socket::ptr accept();

    // 绑定地址
    bool bind(const Address::ptr addr);
    // 连接地址
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    // 监听socket
    bool listen(int backlog = SOMAXCONN);
    // 关闭socket
    bool close();

    // 发送数据
    // 返回值: >0 成功, =0 socket被关闭, <0 socket出错
    int send(const void* buffer, size_t length, int flags = 0);
    int send(const iovec* buffers, size_t length, int flags = 0);
    // 发送数据到目标地址
    int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    // 接收数据
    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffers, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    // 获取远端地址
    Address::ptr getRemoteAddress();
    // 获取本地地址
    Address::ptr getLocalAddress();

    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }

    bool isConnected() const { return m_isConnected; }
    bool isValid() const;
    int getError();

    std::ostream& dump(std::ostream& os) const;
    virtual std::string toString() const;
    
    int getSocket() const { return m_sock; };

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

private:
    void initSock();
    void newSock();
    bool init(int sock);

private:
    int m_sock;
    // 协议簇
    int m_family;
    // 类型
    int m_type;
    // 协议
    int m_protocol;
    bool m_isConnected;

    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};

std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif
