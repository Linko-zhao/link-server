#ifndef __LINKO_SCHEDULER_H__
#define __LINKO_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "mutex.h"
#include "fiber.h"
#include "thread.h"

namespace linko {
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    /*
     * 调度协程
     * fc:协程或函数
     * thread:协程执行的线程id，-1表示任意线程
     */
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }
protected:
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();

    void setThis();
    void run();

    bool hasIdleThreads() { return m_idleThreadCount > 0; }
private:
    //将任务加入到队列中，若任务队列中有任务，则tickle()唤醒
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f), thread(thr) {
        }
        
        FiberAndThread(Fiber::ptr* f, int thr)
            : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f), thread(thr) {
        }

        FiberAndThread(std::function<void()> *f, int thr)
            : thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread() 
            : thread(-1) {
        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex;
    //线程池
    std::vector<Thread::ptr> m_threads;
    //待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;
    std::string m_name;

protected:
    //协程下的线程id数量
    std::vector<int> m_threadIds;
    //线程数量
    size_t m_threadCount = 0;
    //工作线程数量
    std::atomic<size_t> m_activeThreadCount = {0};
    //空闲线程数量
    std::atomic<size_t> m_idleThreadCount = {0};
    //是否正在停止
    bool m_stopping = true;
    //是否自动停止
    bool m_autoStop = false;
    //主线程id(use_caller)
    int m_rootThread = 0;
};
}

#endif
