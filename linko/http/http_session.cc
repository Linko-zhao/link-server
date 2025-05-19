#include "http_session.h"
#include "http_parser.h"

namespace linko {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner) \
    : SocketStream(sock, owner) {
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize(); 
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        // 当前已读取的数据长度
        len += offset;
        // execute会将data指针向后移动nparse个字节
        // nparse为已成功解析的字节数
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        // 因为解析时移动了缓冲区头指针, 所以需要减去已解析部分数据长度
        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    }while (true);

    int64_t length = parser->getContentLength();
    if (length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        // 如果body长度比缓冲区剩余的还大，将缓冲区全部加入
        if (length >= offset) {
            body.append(data, offset);
            len = offset;
        } else {
            body.append(data, length);
            len = length;
        }
        length -= offset;
        // 如果缓冲区数据不满足消息体大小，则继续读取到指定长度
        if (length > 0) {
            if (readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    std::string keep_alive = parser->getData()->getHeader("Connection");
    if (!strcasecmp(keep_alive.c_str(), "keep-alive")) {
        parser->getData()->setClose(false);
    }
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}
