#ifndef __LINKO_TCP_SERVER_H__
#define __LINKO_TCP_SERVER_H__

#include <memory>
#include <functional>
#include <string>

#include "iomanager.h"
#include "socket.h"
#include "address.h"

namespace linko {

class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(linko::IOManager* worker = linko::IOManager::GetThis()
            , linko::IOManager* accept_worker = linko::IOManager::GetThis());
    virtual ~TcpServer();

    // 绑定地址
    virtual bool bind(linko::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs
                        , std::vector<Address::ptr>& fails);

    // bind成功后启动服务
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_isStop; }

protected:
    // 处理新连接的Socket类
    virtual void handleClient(Socket::ptr client);
    // 开始接受连接
    virtual void startAccept(Socket::ptr sock);

private:
    // 监听socket数组
    std::vector<Socket::ptr> m_socks;
    // 新连接的socket工作调度器
    IOManager* m_worker;
    // 服务器socket接收连接的调度器
    IOManager* m_accpetWorker;
    // 接收超时时间
    uint64_t m_recvTimeout;
    // 服务器名称
    std::string m_name;
    // 服务是否停止
    bool m_isStop;

};

}

#endif
