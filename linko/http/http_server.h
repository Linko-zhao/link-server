#ifndef __LINKO_HTTP_SERVER_H__
#define __LINKO_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

#include <memory>

namespace linko{
namespace http{

/*
 * HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false
            , linko::IOManager* worker = linko::IOManager::GetThis()
            , linko::IOManager* accept_worker = linko::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

protected:
    virtual void handleClient(Socket::ptr client);

private:
    // 是否支持长连接
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;

};

}
}

#endif
