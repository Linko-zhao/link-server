#ifndef __LINKO_HTTP_SESSION_H__
#define __LINKO_HTTP_SESSION_H__

#include "../socket_stream.h"
#include "../uri.h"
#include "http.h"

namespace linko {
namespace http {

struct HttpResult {
    typedef std::shared_ptr<HttpResult> ptr;
    enum Error {
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CONNECT_FAIL = 3,
        SEND_CLOSE_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 5,
    };
    HttpResult(int _result
            , HttpResponse::ptr _response
            , const std::string& _error)
        : resutl(_result)
        , response(_response)
        , error(_error) {}
    int resutl;
    HttpResponse::ptr response;
    std::string error;
};

class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection> ptr;

    static HttpResult::ptr DoGet(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                    , const std::string& url
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {}
                                    , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                    , Uri::ptr uri
                                    , uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {}
                                    , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                                    , Uri::ptr uri
                                    , uint64_t timeout_ms);

    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr rsp);
};

}
}

#endif
