#include <iostream>
#include "../linko/http/http_connection.h"
#include "../linko/log.h"
#include "../linko/iomanager.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test_pool() {
    linko::http::HttpConnectionPool::ptr pool(new linko::http::HttpConnectionPool(
                "www.sylar.top", "", 80, 10, 1000 * 30, 5));

    linko::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 300);
            LINKO_LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run() {
    linko::Address::ptr addr = linko::Address::LookupAnyIPAddress("www.sylar.top:80");
    if (!addr) {
        LINKO_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    linko::Socket::ptr sock = linko::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if (!rt) {
        LINKO_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    linko::http::HttpConnection::ptr conn(new linko::http::HttpConnection(sock));
    linko::http::HttpRequest::ptr req(new linko::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    LINKO_LOG_INFO(g_logger) << "req:" << std::endl << *req;
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        LINKO_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    LINKO_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    //LINKO_LOG_INFO(g_logger) << "=======";
    
    //auto r = linko::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    //LINKO_LOG_INFO(g_logger) << "result=" << r->result
        //<< " error=" << r->error
        //<< " rsp=" << (r->response ? r->response->toString() : "");

    //LINKO_LOG_INFO(g_logger) << "=======";
    //test_pool();
}

int main(int argc, char** argv) {
    linko::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
