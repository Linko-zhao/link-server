#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace links {

static links::Logger::ptr g_logger = LINK_LOG_NAME("system");

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    :Scheduler(threads, use_caller, name){
    m_epfd = epoll_create(5000);
    LINK_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    LINK_ASSERT(rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    LINK_ASSERT(rt);

    contextResize(32);

    start();
}

IOManager::~IOManager() {
    
}

int IOManager::addEvent(int fd, Event event);
bool IOManager::delEvent(int fd, Event event);
bool IOManager::cancelEvent(int fd, Event event);

bool IOManager::cancelAll(int fd);
IOManager* IOManager::GetThis();

}
