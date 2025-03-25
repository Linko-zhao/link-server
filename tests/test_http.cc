#include "../linko/http/http.h"
#include "../linko/log.h"
#include <iostream>

void test_request() {
    linko::http::HttpRequest::ptr req(new linko::http::HttpRequest);
    req->setHeader("host", "www.bing.com");
    req->setBody("hello linko");

    req->dump(std::cout) << std::endl;
}

void test_response() {
    linko::http::HttpResponse::ptr rsp(new linko::http::HttpResponse);
    rsp->setHeader("X-X", "linko");
    rsp->setBody("hello linko");
    rsp->setStatus((linko::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
