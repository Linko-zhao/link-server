#include "hook.h"
#include <dlfcn.h>

#include "config.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"

linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

namespace linko {

static linko::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    linko::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
                LINKO_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                        << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, 
        uint32_t event, int timeout_so, Args&&... args) {
    //是否按原函数执行
    if (!linko::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClosed()) {
        //坏文件描述符
        errno = EBADF;
        return -1;
    }

    //非socket 或 用户设置了非阻塞
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    //先执行原函数，若函数返回值有效就直接返回
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    
    //系统中断则重试
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }

    //阻塞状态
    if (n == -1 && errno == EAGAIN) {
        linko::IOManager* iom = linko::IOManager::GetThis();
        linko::Timer::ptr timer;
        //tinfo弱指针，可判断tinfo是否已销毁
        std::weak_ptr<timer_info> winfo(tinfo);

        //设置了超时时间
        if (timeout != (uint64_t)-1) {
            timer = iom->addConditionTimer(timeout, [winfo, fd, iom, event](){
                        auto t = winfo.lock();
                        //tinfo失效 或 设置了错误
                        if (!t || t->cancelled) {
                            return;
                        }
                        //没错误设置为超时
                        t->cancelled = ETIMEDOUT;
                        iom->cancelEvent(fd, (linko::IOManager::Event)(event));
                    }, winfo);
        }

        int rt = iom->addEvent(fd, (linko::IOManager::Event)(event));
        //添加失败，取消定时器
        if (rt == -1) {
            LINKO_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
        } else {
            /*
             * 添加成功后，把执行时间让出
             * 以下两种情况会从此处返回继续执行：
             *  1. 超时，timer的cancelEvent调用triggerEvent会唤醒
             *  2. addEvent数据返回了
             */
            linko::Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }

            //定时器超时失败
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }

            //数据返回，重新操作
            goto retry;
        }
    }

    return n;
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if (!linko::t_hook_enable) {
        return sleep_f(seconds);
    }

    linko::Fiber::ptr fiber = linko::Fiber::GetThis();
    linko::IOManager* iom = linko::IOManager::GetThis();

    //iom->addTimer(seconds * 1000, [iom, fiber](){
        //iom->schedule(fiber);
    //});
    //iom->addTimer(seconds * 1000, std::bind(&linko::IOManager::schedule, iom, fiber));

    /*
     * bind方法不能绑定模板函数，故需要在函数调用时实力化schedule函数模板参数
     * 此处将schedule方法的两个参数实例化为 Fiber::ptr 和 int 类型
     *
     * iom为函数绑定到实例化对象，fiber和-1为两个参数
     */
    iom->addTimer(seconds * 1000, std::bind(
        (void(linko::Scheduler::*)
            (linko::Fiber::ptr, int thread))&linko::IOManager::schedule,
        iom, fiber, -1
    ));
    linko::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if (!linko::t_hook_enable) {
        return usleep_f(usec);
    }

    linko::Fiber::ptr fiber = linko::Fiber::GetThis();
    linko::IOManager* iom = linko::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind(
        (void(linko::Scheduler::*)
            (linko::Fiber::ptr, int thread))&linko::IOManager::schedule,
        iom, fiber, -1
    ));

    linko::Fiber::YieldToHold();
 
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!linko::t_hook_enable) {
        return nanosleep(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    linko::Fiber::ptr fiber = linko::Fiber::GetThis();
    linko::IOManager* iom = linko::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind(
        (void(linko::Scheduler::*)
            (linko::Fiber::ptr, int thread))&linko::IOManager::schedule,
        iom, fiber, -1
    ));

    linko::Fiber::YieldToHold();

    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!linko::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    linko::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
    if (!linko::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClosed()) {
        errno = EBADF;
        return  -1;
    }
    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }
    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    } else if (n != -1 || errno != EINPROGRESS){
        return n;
    }

    linko::IOManager* iom = linko::IOManager::GetThis();
    linko::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, linko::IOManager::WRITE);
                }, winfo);
    }

    int rt = iom->addEvent(fd, linko::IOManager::WRITE);
    if (rt == 0) {
        linko::Fiber::YieldToHold();
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    } else {
        if (timer) {
            timer->cancel();
        }
        LINKO_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, linko::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", linko::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        linko::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", linko::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", linko::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", linko::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", linko::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", linko::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", linko::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", linko::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", linko::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", linko::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", linko::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!linko::t_hook_enable) {
        return close_f(fd);
    }

    linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = linko::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        linko::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
    case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
        break;
    case F_GETFL:
        {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
                return arg;
            }
            if (ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
        }
        break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETSIG:
    case F_SETOWN:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ:
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ:
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
        break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
        {
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
    case F_GETOWN_EX:
    case F_SETOWN_EX:
        {
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
    default:
        va_end(va);
        return fcntl_f(fd, cmd);
    }
}

int ioctl(int fd, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
           return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if (!linko::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            linko::FdCtx::ptr ctx = linko::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval* tv = (const timeval*)optval;
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
