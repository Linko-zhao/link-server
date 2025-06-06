#include "http_connection.h"
#include "http_parser.h"
#include "../log.h"

#include <string>
#include <sstream>

namespace linko {
namespace http {

static linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner) {
    m_createTime = linko::GetCurrentMS();
}

HttpConnection::~HttpConnection() {
    LINKO_LOG_INFO(g_logger) << "~HttpConnection";
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize(); 
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
        len += offset;
        // httpclient_parser_execute解析时数据最后需要用'\0'结尾
        data[len] = '\0';
        size_t nparse = parser->execute(data, len, false);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    }while (true);

    auto& client_parser = parser->getParser();
    // 是否为chunked分块
    if (client_parser.chunked) {
        std::string body;
        int len = offset;
        do {
            // 继续读取下一块数据报头
            do {
                int rt = read(data + len, buff_size - len);
                if (rt <= 0) {
                    close();
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if (parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if (len == (int)buff_size) {
                    close();
                    return nullptr;
                }
            } while (!parser->isFinished());
            len -= 2;

            LINKO_LOG_INFO(g_logger) << "content_len=" << client_parser.content_len;

            // body长度小于缓冲区剩余数据长度
            if (client_parser.content_len <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len
                        , len - client_parser.content_len);
                len -= client_parser.content_len;
            } else {
                // 将缓冲区数据全部加入, 并继续读取剩余body
                body.append(data, len);
                int left = client_parser.content_len - len;
                while (left > 0) {
                    int rt = read(data, left > (int)buff_size ? buff_size : left);
                    if (rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                len = 0;
            }
        } while (!client_parser.chunks_done);
        parser->getData()->setBody(body);
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
                    close();
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

HttpResult::ptr HttpConnection::DoGet(const std::string& url
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalide url:" + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalide url: " + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                         , const std::string& url
                         , uint64_t timeout_ms
                         , const std::map<std::string, std::string>& headers
                         , const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL
                , nullptr, "invalide url: " + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                         , Uri::ptr uri
                         , uint64_t timeout_ms
                         , const std::map<std::string, std::string>& headers
                         , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for (auto& i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    // 如果没有host, 则用uri的host
    if (!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                             , Uri::ptr uri
                             , uint64_t timeout_ms) {
    // 通过uri创建address
    Address::ptr addr = uri->createAddress();
    if (!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalide host: " + uri->getHost());
    }
    // 创建TCPsocket
    Socket::ptr sock = Socket::CreateTCP(addr);
    if (!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + std::to_string(errno)
                        + " errstr=" + std::string(std::strerror(errno)));
    }
    // 发起连接请求
    if (!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }
    // 设置接收超时时间
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    // 发送请求报文
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                , nullptr, "send request socket error errno=" + std::to_string(errno)
                + " errstr=" + std::string(std::strerror(errno)));
    }
    // 接收响应报文
    auto rsp = conn->recvResponse();
    if (!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                , nullptr, "recv response timeout: " + addr->toString()
                + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

HttpConnectionPool::HttpConnectionPool(const std::string& host
                                    , const std::string& vhost
                                    , uint32_t port
                                    , uint32_t max_size
                                    , uint32_t max_alive_time
                                    , uint32_t max_request) 
    : m_host(host)
    , m_vhost(vhost)
    , m_port(port)
    , m_maxSize(max_size)
    , m_maxAliveTime(max_alive_time)
    , m_maxRequest(max_request) {
}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = linko::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while (!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        // 不在连接状态
        if (!conn->isConnected()) {
            invalid_conns.push_back(conn);
            continue;
        }
        // 超过最大连接时间
        if ((conn->m_createTime + m_maxAliveTime) < now_ms) {
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for (auto i : invalid_conns) {
        delete i;
    }
    m_total -= invalid_conns.size();

    if (!ptr) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if (!addr) {
            LINKO_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock) {
            LINKO_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if (!sock->connect(addr)) {
            LINKO_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }
        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
                                , std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    if (!ptr->isConnected()
            || ((ptr->m_createTime + pool->m_maxAliveTime) <= linko::GetCurrentMS())
            || (ptr->m_request >= pool->m_maxRequest)) {
        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}
    
HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                     , uint64_t timeout_ms
                     , const std::map<std::string, std::string>& headers
                     , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                         , const std::string& url
                         , uint64_t timeout_ms
                         , const std::map<std::string, std::string>& headers
                         , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for (auto& i : headers) {
        if (strcasecmp(i.first.c_str(), "connection") == 0) {
            if (strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }

        if (!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }

        req->setHeader(i.first, i.second);
    }
    if (!has_host) {
        if (m_vhost.empty()) {
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);

}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                         , Uri::ptr uri
                         , uint64_t timeout_ms
                         , const std::map<std::string, std::string>& headers
                         , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       << (uri->getFragment().empty() ? "" : "#")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                         , uint64_t timeout_ms) {
    auto conn = getConnection();
    if (!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION_FAIL
                , nullptr, "pool host: " + m_host + " port:" + std::to_string(m_port));
    }
    auto sock = conn->getSocket();
    if (!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALIDE_CONNECTION
                , nullptr, "pool host: " + m_host + " port:" + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                , nullptr, "send request socket error errno=" + std::to_string(errno)
                + " errstr=" + std::string(std::strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if (!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                + " timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}


}
}
