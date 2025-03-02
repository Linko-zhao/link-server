#include "../linko/socket.h"
#include "../linko/links.h"
#include "../linko/iomanager.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void test_socket() {
    linko::IPAddress::ptr addr = linko::Address::LookupAnyIPAddress("www.baidu.com");
    if (!addr) {
        LINKO_LOG_ERROR(g_logger) << "get address fail";
        return;
    }
    LINKO_LOG_INFO(g_logger) << "get address:" << addr->toString();

    linko::Socket::ptr sock = linko::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)) {
        LINKO_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    }        
    LINKO_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        LINKO_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    
    if (rt <= 0) {
        LINKO_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    LINKO_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char** argv) {
    linko::IOManager iom;
    iom.schedule(test_socket);
    return 0;
}
