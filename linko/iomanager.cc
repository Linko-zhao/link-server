#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace linko {

static linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            LINKO_ASSERT2(false, "getContext");
    }
}


void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.cb = nullptr;
    ctx.fiber.reset();
}


void IOManager::FdContext::triggeredEvent(Event event) {
    LINKO_ASSERT(events & event);
    //触发该事件就将该事件从注册事件中删除
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
}


IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    LINKO_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    LINKO_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    //注册读事件，设置边缘触发模式
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    //对打开的文件描述符执行操作
    //F_SETFL：设置文件状态标志，O_NONBLOCK：非阻塞模式I/O
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    LINKO_ASSERT(!rt);

    //将pipe的读端注册到epoll
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    LINKO_ASSERT(!rt);

    //初始化socket事件上下文数组
    contextResize(32);

    start();
}


IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}


void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}


int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);

    //从m_fdContexts中获取对应的FdContext
    if (m_fdContexts.size() > (size_t)fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        //扩充容器
        contextResize(m_fdContexts.size() * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //一般同一个句柄添加事件相同，可能为两个线程在操纵同一个句柄
    if (fd_ctx->events & event) {
        LINKO_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
            << " event=" << event
            << " fd_ctx.event=" << fd_ctx->events;
        LINKO_ASSERT(!(fd_ctx->events & event));
    }

    //已有注册事件则为修改事件，否则为添加
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    //设置边缘触发，添加原有事件以及要注册事件
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    //注册事件
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        LINKO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << ") (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;

    //更新上下文事件
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    LINKO_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        LINKO_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }

    return 0;
}


bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t)fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    //将事件从注册事件中删除
    Event new_events = (Event)(fd_ctx->events & ~event);
    //仍有事件则修改，否则删除
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        LINKO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;

    //修改注册事件，重置上下文
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}


bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t)fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        LINKO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    //删除之前触发一次事件
    fd_ctx->triggeredEvent(event);
    --m_pendingEventCount;
    return true;
}


bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_fdContexts.size() <= (size_t)fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events)) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        LINKO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    //触发所有事件
    if (fd_ctx->events & READ) {
        fd_ctx->triggeredEvent(READ);
        --m_pendingEventCount;
    }

    if (fd_ctx->events & WRITE) {
        fd_ctx->triggeredEvent(WRITE);
        --m_pendingEventCount;
    }

    LINKO_ASSERT(fd_ctx->events == 0);
    return true;

}


IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


void IOManager::tickle() {
    //没有在执行的idle线程
    if (!hasIdleThreads()) {
        return;
    }
    //边缘触发，有任务时，往pipe里写入一个字节数据，就会唤醒epoll_wait
    int rt = write(m_tickleFds[1], "T", 1);
    LINKO_ASSERT(rt == 1);
} 

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    //定时器为空，等待执行事件数量为0，scheduler可以停止
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

/*
 * idel协程应该关注两件事，一是有没有新的调度任务，二是当前注册的所有IO事件有没有触发
 */
void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete [] ptr;
    });

    /*
     * 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，
     * 对应的操作是tickle或注册的IO事件就绪。
     */
    while (true) {
        //下一个任务要执行的时间
        uint64_t next_timeout = 0;
        //获取下一个任务时间，并判断是否停止
        if (stopping(next_timeout)) {
            LINKO_LOG_INFO(g_logger) << "name=" << getName() 
                << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            //如果有定时器任务，最多休眠MAX_TIMEOUT
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                    ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }

            /*
             * 在此阻塞，以下3种情况会唤醒：
             *  1. 超时时间到
             *  2. 关注的socket有数据到达
             *  3. 通过tickle唤醒
             */
            rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
            if (rt < 0 && errno == EINTR) {
                //发生系统中断EINTR，需要重新尝试epoll_wait
            } else {
                break;
            }
        } while (true);

        std::vector<std::function<void()> > cbs;
        //获取超时任务，并放入任务队列
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            //获得的信息来自pipe
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy;
                //将第一个字节无效数据取出
                while (read(m_tickleFds[0], &dummy, 1) == 1) ;
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            /*
             * EPOLLERR：出错，比如读写端已关闭的pipe
             * EPOLLHUP：socket套接字对端关闭
             * 出现这两种事件，应该同时触发fd的读和写事件，
             * 否则有可能出现注册的事件永远不执行的情况。
             */
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            //剔除已发生的事件，将剩下的事件重新加入Epoll
            //如剩下的事件为NONE，则删除
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                LINKO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if (real_events & READ) {
                fd_ctx->triggeredEvent(READ);
                --m_pendingEventCount;
            }

            if (real_events & WRITE) {
                fd_ctx->triggeredEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        
        //一旦处理完所有的事，idle协程让出，可以让调度协程重新检查是否有新任务要调度
        //triggeredEvent仅将对应的任务加入的调度队列，需要等idle协程退出才会执行
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }    
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}
