#ifndef __LINKO_ADDRESS_H__
#define __LINKO_ADDRESS_H__

#include <memory>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <iostream>
#include <map>

namespace linko {

class IPAddress;

class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    // 通过sockaddr指针创建Address
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    // 通过host地址返回对应条件的所有Address
    // result: 返回满足条件的Address
    // host: 域名、服务器名等
    // family: 协议族 AF_INET\AF_INET6\AF_UNIX
    // type: 套接字类型SOCK_STREAM\SOCK_DGRAM
    // protocol: 协议 IPPROTO_TCP\IPPROTO_UDP
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    // 返回对应条件的任意Address
    static Address::ptr LookupAny(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);
    // 返回对饮条件的任意IPAddress
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    // 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
    static bool GetInterfaceAddress(
            std::multimap<std::string, std::pair<Address::ptr, uint32_t> >&result
            , int family = AF_UNSPEC);
    
    // 获取指定网卡的地址和子网掩码位数
    static bool GetInterfaceAddress(
            std::vector<std::pair<Address::ptr, uint32_t> >&result
            , const std::string& iface, int family = AF_UNSPEC);

    virtual ~Address() {}

    int getFamily() const;

    virtual const sockaddr* getAddr() const = 0;
    virtual sockaddr* getAddr() = 0;
    virtual socklen_t getAddrLen() const = 0;

    // 可读性输出
    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString();
    
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    // 获取该地址的广播地址
    // prefix_len: 子网掩码位数
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    // 获取该地址的网段
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    // 获取子网掩码地址
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};

class IPv4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;
    static IPv4Address::ptr Create(const char* address, uint16_t port);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
    
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(const uint8_t address[16], uint16_t port);
    
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::ostream& insert(std::ostream& os) const override;

private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(const sockaddr& addr);
    UnknownAddress(int family);

    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr m_addr;
};

}

#endif
