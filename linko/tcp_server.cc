#include "tcp_server.h"
#include "config.h"
#include "log.h"

namespace linko {

static linko::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    linko::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
            "tcp server read timeout");

static linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

TcpServer::TcpServer(linko::IOManager* worker 
            , linko::IOManager* accept_worker)
    : m_worker(worker)
    , m_accpetWorker(accept_worker)
    , m_recvTimeout()
    , m_name("linko/1.0.0")
    , m_isStop(true) {
}

TcpServer::~TcpServer() {
    for (auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

bool TcpServer::bind(linko::Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                    , std::vector<Address::ptr>& fails) {
    for (auto& addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if (!sock->bind(addr)) {
            LINKO_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            LINKO_LOG_ERROR(g_logger) << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    if (!fails.empty()) {
        m_socks.clear();
        return false;
    }
    
    for (auto& i : m_socks) {
        LINKO_LOG_INFO(g_logger) << "server bind success: " << *i;
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock) {
    while (!m_isStop) {
        Socket::ptr client = sock->accept();
        if (client) {
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TcpServer::handleClient,
                        shared_from_this(), client));
        } else {
            LINKO_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

bool TcpServer::start() {
    if (!m_isStop) {
        return true;
    }
    m_isStop = false;

    for (auto& sock : m_socks) {
        m_accpetWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    // 仅获取this无法保证对象不被销毁，可能成为悬空指针
    m_accpetWorker->schedule([this, self]() {
        for (auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    LINKO_LOG_INFO(g_logger) << "handleClient: " << *client;
}

}
