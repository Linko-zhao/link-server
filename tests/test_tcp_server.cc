#include "../linko/tcp_server.h"
#include "../linko/iomanager.h"
#include "../linko/log.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void run() {
    auto addr = linko::Address::LookupAny("0.0.0.0:8033");
    auto addr2 = linko::UnixAddress::ptr(new linko::UnixAddress("/tmp/unix_addr"));
    LINKO_LOG_INFO(g_logger) << *addr;
    std::vector<linko::Address::ptr> addrs;
    addrs.push_back(addr);
    addrs.push_back(addr2);
    
    linko::TcpServer::ptr tcp_server(new linko::TcpServer);
    std::vector<linko::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}

int main(int argc, char** argv) {
    linko::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
