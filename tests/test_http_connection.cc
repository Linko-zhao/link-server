#include <iostream>
#include "../linko/http/http_connection.h"
#include "../linko/log.h"
#include "../linko/iomanager.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void run() {
    linko::Address::ptr addr = linko::Address::LookupAnyIPAddress("www.bing.com:80");
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
    req->setHeader("host", "www.bing.com");
    LINKO_LOG_INFO(g_logger) << "req:" << std::endl << *req;
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        LINKO_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    LINKO_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;
}

int main(int argc, char** argv) {
    linko::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
