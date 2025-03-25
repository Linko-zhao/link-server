#ifndef __LINKO_HTTP_SESSION_H__
#define __LINKO_HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"

namespace linko {
namespace http {


class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr rsp);
};

}
}

#endif
