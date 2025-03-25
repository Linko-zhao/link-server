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

    virtual bool bind(linko::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs
                        , std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_isStop; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

private:
    std::vector<Socket::ptr> m_socks;
    IOManager* m_worker;
    IOManager* m_accpetWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_isStop;

};

}

#endif
