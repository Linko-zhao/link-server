#ifndef __LINKO_FIBER_H__
#define __LINKO_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace linko {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
private:
    //构造主协程
    Fiber();

public:
            
    //构造子协程
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    void reset(std::function<void()> cb);

    //从主协程切换到当前协程
    void call();

    //当前线程切换到主协程
    void back();

    //从调度器主协程切换到当前协程
    void swapIn();

    //从当前协程切换到调度器主协程
    void swapOut();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

public:
    static void SetThis(Fiber* f);
    static Fiber::ptr GetThis();
    static void YieldToReady();
    static void YieldToHold();
    
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    //协程运行栈指针
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};

}

#endif
