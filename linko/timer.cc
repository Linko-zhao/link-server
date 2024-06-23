#include "timer.h"
#include "util.h"

namespace linko {

bool Timer::Comparator::operator()(const Timer::ptr &lhs, 
                                    const Timer::ptr &rhs) const {
    //比较定时器的智能指针的大小(按执行时间排序)
    if (!lhs && !rhs) {
        return false;
    }

    if (!lhs) {
        return true;
    }

    if (!rhs) {
        return false;
    }

    if (lhs->m_next < rhs->m_next) {
        return true;
    }

    if (rhs->m_next < lhs->m_next) {
        return false;
    }

    return lhs.get() < rhs.get();
}


Timer::Timer(uint64_t ms, std::function<void()> cb,
            bool recurring, TimerManager* manager) 
    : m_recurring(recurring)
    , m_ms(ms)
    , m_cb(cb)
    , m_manager(manager) {
    m_next = linko::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) 
    : m_next(next){
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = linko::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = linko::GetCurrentMS();
    } else {
        //设置为当时创建时的起始时间
        start = m_next - m_ms;
    }
    //更新时间
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;    
}

TimerManager::TimerManager() {
    m_previousTime = linko::GetCurrentMS();
}

TimerManager::~TimerManager() {
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, 
                                bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    //如果该定时器是超时时间最短 && 没有设置触发onTimerInsertedAtFront
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if (at_front) {
        //iomanger中执行了一次tickle
        onTimerInsertedAtFront();
    }
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    //weak_ptr的lock方法可以返回一个shared_ptr类型指针，如果weak_ptr无效，则返回空
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, 
                            std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    //无定时器，返回最大值
    if (m_timers.empty()) {
        return ~0ull;
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = linko::GetCurrentMS();
    if (now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = linko::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (m_timers.empty()) {
        return;
    }

    bool rollover = detectClockRollover(now_ms);
    //服务器时间正常 && 第一个计时器未超时
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    //若服务器时间改动，则所有Timer都视为过期
    //否则返回第一个>= now_ms的迭代器，在此迭代器之前的定时器全部超时
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    //筛选出执行时间与当前时间相同的定时器
    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    //取出超时的部分
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);

    //获取所有超时的任务
    cbs.reserve(expired.size());
    for (auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        //如果是循环定时器，则设置新的执行时间，重新加入定时器集合中
        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    //如果当前时间比上次执行时间还小 && 小于一个小时的时间，相当于时间倒流了
    if (now_ms < m_previousTime &&
        now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}
