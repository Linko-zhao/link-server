#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace linko {

static Logger::ptr g_logger = LINKO_LOG_NAME("system");

//用于生成协程id
static std::atomic<uint64_t> s_fiber_id {0};
//协程数
static std::atomic<uint64_t> s_fiber_count {0};

//当前协程
static thread_local Fiber* t_fiber = nullptr;
//主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

//协程栈大小约定为1MB
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

//创建和释放运行栈
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        LINKO_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;

    LINKO_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) 
    : m_id(++s_fiber_id) 
    , m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    //将当前协程上下文存入m_ctx中
    if (getcontext(&m_ctx)) {
        LINKO_ASSERT2(false, "getcontext");
    }

    //执行完当前context之后退出程序
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    LINKO_LOG_DEBUG(g_logger) << "Fiber::Fiber id:" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    //子协程是否存在
    if (m_stack) {
        LINKO_LOG_INFO(g_logger) << "Fiber id:" << m_id << " m_state:" << m_state;
        LINKO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        //主协程释放要保证没有任务且正在运行
        LINKO_ASSERT(!m_cb);
        LINKO_ASSERT(m_state == EXEC);

        //如果当前协程为主协程，则置空
        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }

    LINKO_LOG_DEBUG(g_logger) << "Fiber::~Fiber id:" << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    LINKO_ASSERT(m_stack);
    //当前协程不在准备和运行态
    LINKO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        LINKO_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    LINKO_ASSERT(m_state != EXEC);
    m_state = EXEC;

    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        LINKO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());

    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        LINKO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapIn() {
    SetThis(this);
    LINKO_ASSERT(m_state != EXEC);
    m_state = EXEC;
    
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        LINKO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());

    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        LINKO_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    //返回当前协程
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    //如果当前线程不存在协程，则创建主协程
    Fiber::ptr main_fiber(new Fiber);
    LINKO_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    LINKO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LINKO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() 
            << " fiber_id:" << cur->getId()
            << std::endl 
            << linko::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LINKO_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id:" << cur->getId()
            << std::endl 
            << linko::BacktraceToString();
    }

    //获得裸指针
    auto raw_ptr = cur.get();
    //引用-1，防止fiber释放不掉
    cur.reset();
    
    raw_ptr->swapOut();

    LINKO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    LINKO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        LINKO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() 
            << " fiber_id:" << cur->getId()
            << std::endl 
            << linko::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LINKO_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id:" << cur->getId()
            << std::endl 
            << linko::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();

    raw_ptr->back();

    LINKO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}
