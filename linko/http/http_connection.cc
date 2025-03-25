#include "http_connection.h"
#include "http_parser.h"

namespace linko {
namespace http {

HttpConnection::HttpConnection(Socket::ptr sock, bool owner) \
    : SocketStream(sock, owner) {
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize(); 
    //uint64_t buff_size = 200;
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    }while (true);

    auto client_parser = parser->getParser();
    if (client_parser.chunked) {
        do {
        
        }while (client_parser.chunks_done);
    } else {
        int64_t length = parser->getContentLength();
        if (length > 0) {
            std::string body;
            body.resize(length);

            int len = 0;
            if (length >= offset) {
                body.append(data, offset);
                len = offset;
            } else {
                body.append(data, length);
                len = length;
            }
            length -= offset;
            if (length > 0) {
                if (readFixSize(&body[len], length) <= 0) {
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}
