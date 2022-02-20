#include "timer.h"

#include <climits>
#include "utils.h"
#include "log.h"

namespace qff {
    

bool Timer::Comparator::operator()(const Timer::ptr lhs
                                , const Timer::ptr rhs) const noexcept {
    if(!lhs)
        return true;
    if(!rhs)
        return false;
    if(lhs->m_next_time < rhs->m_next_time)
        return true;
    else 
        return false;
}

Timer::Timer(int ms, CallBackType cb, bool recurring, TimerManager* manager) 
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
    m_next_time = GetCurrentMS() + ms;
}

Timer::Timer(int next_time) 
    :m_next_time(next_time) {

}

int Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_manager || !m_cb)
        return -1;

    auto &timers = m_manager->m_timers;

    auto it = timers.find(shared_from_this());
    if(it == timers.end())
        return -1;
    timers.erase(it);
    m_cb = nullptr;

    return 0;
}

int Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_manager || !m_cb)
        return -1;
    
    auto &timers = m_manager->m_timers;
    auto it = timers.find(shared_from_this());
    if(it == timers.end())
        return -1;
    timers.erase(it);
    m_next_time = GetCurrentMS() + m_ms;
    m_manager->add_timer(shared_from_this());
    return 0;
}

int Timer::reset(int ms, bool from_now) {
    if(ms == m_ms && !from_now)
        return 0;

    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_manager || !m_cb)
        return -1;
    
    auto &timers = m_manager->m_timers;

    auto it = timers.find(shared_from_this());
    if(it == timers.end())
        return -1;
    timers.erase(it);

    int start;
    if(from_now)
        start = GetCurrentMS();
    else
        start = m_next_time - m_ms;
    m_ms = ms;
    m_next_time = start + ms;

    timers.insert(shared_from_this());
    return 0;
}

TimerManager::TimerManager() noexcept {
    m_previous_time.store(GetCurrentMS());
}

TimerManager::~TimerManager() noexcept {

}

Timer::ptr TimerManager::add_timer(int ms, Timer::CallBackType cb, bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    add_timer(timer);
    return timer;
}

void TimerManager::add_timer(Timer::ptr timer) {
    auto it = m_timers.insert(timer);
    bool tickle = (it.first == m_timers.begin()) && !m_tickled;
    if(!tickle)
        return;
    m_tickled.store(true);
    m_mutex.unlock();
    this->on_timer_inserted_into_front();
}

static void OnTimer(std::weak_ptr<void> cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = cond.lock();
    if(!tmp)
        return;
    cb();
}

Timer::ptr TimerManager::add_cond_timer(int ms, Timer::CallBackType cb, std::weak_ptr<void> cond, bool recurring) {
    return this->add_timer(ms, std::bind(&OnTimer, cond, cb), recurring);
}

int TimerManager::get_next_time() noexcept {
    m_tickled.store(false);
    int  now_time = GetCurrentMS();
    this->detect_clock_rollover(now_time);

    RWMutexType::ReadLock lock(m_mutex);
    if(m_timers.empty())
        return INT_MAX;
    
    const Timer::ptr& next = *m_timers.begin();
    if(now_time >= next->m_next_time)
        return 0;
    
    return next->m_next_time;
}

bool TimerManager::has_timer() noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

void TimerManager::detect_clock_rollover(int now_time) noexcept {
    if(LIKELY(now_time >= m_previous_time ||
            now_time >= (m_previous_time - 60 * 60 *1000))) {
        m_previous_time.store(now_time);
        return;
    }
    
    int distance = now_time - m_previous_time;
    RWMutex::WriteLock lock(m_mutex);
    for (auto it = m_timers.begin(); it != m_timers.end(); ++it)
        (*it)->m_next_time -= distance;
    lock.unlock();

    m_previous_time.store(now_time);
}

std::vector<Timer::CallBackType> TimerManager::list_expired_cb() {
    int now_time = GetCurrentMS();

    RWMutexType::ReadLock lock1(m_mutex);
    if(m_timers.empty())
        return std::vector<Timer::CallBackType>();
    lock1.unlock();

    std::vector<Timer::CallBackType> expired_cbs;
    std::vector<Timer::ptr> cache_timers;
    
    RWMutexType::WriteLock lock2(m_mutex);

    Timer::ptr now_timer(new Timer(now_time));
    auto end_pos = m_timers.upper_bound(now_timer);
   
    for(auto it = m_timers.begin(); it != end_pos; ++it) {
        auto timer = *it;
        expired_cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next_time = now_time + timer->m_ms;
            cache_timers.push_back(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }

    m_timers.erase(m_timers.begin(), end_pos);
    for(auto timer : cache_timers) {
        this->add_timer(timer);
    }
    return expired_cbs;
}

void TimerManager::timer_manager_stop() noexcept {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_timers.begin(); it != m_timers.end();) {
        if((*it)->m_recurring)
            it = m_timers.erase(it);
        else 
            ++it;
    }
}

} // namespace qff
