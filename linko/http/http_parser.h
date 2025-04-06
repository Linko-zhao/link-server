#ifndef __LINKO_HTTP_PARSER_H__
#define __LINKO_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace linko {
namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();
    void setError(int v) { m_error = v; }

    HttpRequest::ptr getData() { return m_data; }

    // 获取消息体长度
    uint64_t getContentLength();
    const http_parser& getParser() const { return m_parser; }

public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    // 1000: invalid method, 1001: invalid version, 1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();
    size_t execute(char* data, size_t len, bool chunck);
    int isFinished();
    int hasError();
    void setError(int v) { m_error = v; }

    HttpResponse::ptr getData() { return m_data; }

    uint64_t getContentLength();
    const httpclient_parser& getParser() const { return m_parser; }

public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};

}
}

#endif
