#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace links {
static links::Logger::ptr g_logger = LINK_LOG_NAME("system");

//协程调度器
static thread_local Scheduler* t_scheduler = nullptr;
//线程主协程
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    : m_name(name) {
    LINK_ASSERT(threads > 0);

    //use_caller决定是否将协程调度线程也加入调度器中
    if (use_caller) {
        /*
         * Note
         * 由于调度器所在线程也需要用于处理其他协程任务，
         * 故该线程中不只是运行协程调度器一个任务，所以
         * 需要创建一个主协程用于执行协程调度器任务
         * （即替代原线程只执行一个调度器的任务）
         */
        links::Fiber::GetThis();
        --threads;

        LINK_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        //将此fiber设置为use_caller，协程会与Fiber::CallerMainFunc绑定
        //非静态成员函数需要传this指针作为第一个参数，通过std::bind绑定
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        links::Thread::SetName(m_name);

        //设置当前线程的主协程为m_rootFiber
        //m_rootFiber为当前线程的主协程
        t_fiber = m_rootFiber.get();
        m_rootThread = links::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    LINK_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_fiber;
}

void Scheduler::start() {
    LINK_LOG_DEBUG(g_logger) << "start";
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    LINK_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                    , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }

    lock.unlock();

    /*
     * swap会将线程的主协程与当前协程切换
     * 当使用use_caller时，t_fiber=m_rootFiber，call是将当前协程与主协程交换
     * 为了确保在启动后仍有任务加入任务队列中，所以在stop中做该协程的启动，
     * 这样就不会漏掉任务队列中的任务
     */
    //if (m_rootFiber) {
    //    m_rootFiber->call();
    //    //m_rootFiber->swapIn();
    //    LINK_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //}
}

void Scheduler::stop() {
    LINK_LOG_INFO(g_logger) << "stop";
    m_autoStop = true;

    //1.主协程存在，即使用了use_caller
    //2.只剩当前线程
    //3.主协程状态为终止或初始化
    if (m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        LINK_LOG_INFO(g_logger) << this << " schedule stopped";
        m_stopping = true;
        
        if (stopping()) {
            return;
        }
    }
    
    //bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {
        LINK_ASSERT(GetThis() == this);
    } else {
        LINK_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    //if (stopping()) {
    
    //}

    if (m_rootFiber) {
        //while (!stopping()) {
        //    if (m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        LINK_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    LINK_LOG_DEBUG(g_logger) << "run";
    setThis();
    if (links::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                if (it->thread != -1 && it->thread != links::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                LINK_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }

        if (tickle_me) {
            tickle();
        }

        if (ft.fiber && ft.fiber->getState() != Fiber::TERM 
                        && ft.fiber->getState() != Fiber::EXCEPT) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                LINK_LOG_INFO(g_logger) << "idle fiber terminate";
                tickle();
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM
             && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}


void Scheduler::tickle() {
    LINK_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop 
        && m_stopping
        && m_fibers.empty() 
        && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    LINK_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        links::Fiber::YieldToHold();
    }
}

}
