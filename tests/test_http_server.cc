#include "../linko/http/http_server.h"
#include "../linko/log.h"

static linko::Logger::ptr g_logger = LINKO_LOG_ROOT();

void run() {
    linko::http::HttpServer::ptr server(new linko::http::HttpServer);
    linko::Address::ptr addr = linko::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/linko/xx", [](linko::http::HttpRequest::ptr req
                , linko::http::HttpResponse::ptr rsp
                , linko::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
        });

    sd->addGlobServlet("/linko/*", [](linko::http::HttpRequest::ptr req
                , linko::http::HttpResponse::ptr rsp
                , linko::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
        });
    server->start();
}

int main(int argc, char** argv) {
    linko::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
