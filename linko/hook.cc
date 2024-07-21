#include "hook.h"
#include <dlfcn.h>

#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"

linko::Logger::ptr g_logger = LINKO_LOG_NAME("system");

namespace linko {

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

struct _HookIniter {
    _HookIniter() {
        hook_init();
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

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_f(sockfd, addr, addrlen);
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

}

int ioctl(int d, unsigned long int request, ...) {

}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {

}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {

}

}
