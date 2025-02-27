#include "socket.h"
#include "fd_manager.h"
#include "log.h"
#include "macro.h"


namespace linko {

static linko::Logger::ptr g_logger = LINKO_LOG_NAME("SYSTEM");

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected (false) {

}

Socket::~Socket() {
    close();
}

int64_t Socket::getSendTimeout(){
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

bool Socket::setSendTimeout(int64_t v){
    struct timeval tv{ int(v / 1000), int(v % 1000 * 1000) };
    return setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout(){
     FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if (ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

bool Socket::setRecvTimeout(int64_t v){
    struct timeval tv{ int(v / 1000), int(v % 1000 * 1000) };
    return setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len){
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if (rt) {
        LINKO_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const void* result, size_t len){
    if (setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        LINKO_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept(){
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if (newsock == -1) {
        LINKO_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
           << errno << " errstr=" << strerror(errno);
        return nullptr; 
    }
    if (sock->init(newsock)) {
        return sock;
    }
}

bool Socket::init(int sock){
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if (ctx && ctx->isSocket() && !ctx->isClosed()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}
bool Socket::bind(const Address::ptr addr){}
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms){}
bool Socket::listen(int backlog){}
bool Socket::close(){}

int Socket::send(const void* buffer, size_t length, int flags){}
int Socket::send(const iovec* buffers, size_t length, int flags){}
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){}
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags){}

int Socket::recv(void* buffer, size_t length, int flags){}
int Socket::recv(iovec* buffers, size_t length, int flags){}
int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags){}
int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags){}

Address::ptr Socket::getRemoteAddress(){}
Address::ptr Socket::getLocalAddress(){}

bool Socket::isValid() const{}
int Socket::getError(){}

std::ostream& Socket::dump(std::ostream& os) const{}
int Socket::getSocket() const{}

bool Socket::cancelRead(){}
bool Socket::cancelWrite(){}
bool Socket::cancelAccept(){}
bool Socket::cancelAll(){}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if (m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if (LINKO_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        LINKO_LOG_ERROR(g_logger) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
    }
}

}
