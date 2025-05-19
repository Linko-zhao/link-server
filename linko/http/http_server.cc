#include "http_server.h"
#include "../log.h"

namespace linko {
namespace http {

static linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, linko::IOManager* worker
                                    , linko::IOManager* accept_worker)
    : TcpServer(worker, accept_worker)
    , m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        // 接收请求报文
        auto req = session->recvRequest();    
        if (!req) {
            LINKO_LOG_WARN(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " client:" << *client;
            break;
        }

        // 创建响应报文
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                    , req->isClose() || !m_isKeepalive));

        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if (!m_isKeepalive || req->isClose()) {
            break;
        }
    }while (true);
    session->close();
}

}
}
