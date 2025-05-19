#include "../linko/http/http_server.h"
#include "../linko/log.h"

linko::Logger::ptr g_logger = LINKO_LOG_ROOT();
linko::IOManager::ptr worker;

void run() {
    g_logger->setLevel(linko::LogLevel::INFO);
    linko::Address::ptr addr = linko::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        LINKO_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    linko::http::HttpServer::ptr http_server(new linko::http::HttpServer(true));
    while (!http_server->bind(addr)) {
        LINKO_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    http_server->start();
}

int main(int argc, char** argv) {
    linko::IOManager iom(3);
    //worker.reset(new linko::IOManager(4, false));
    iom.schedule(run);
    return 0;
}
