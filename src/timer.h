#ifndef __QFF__TIMER_H__
#define __QFF__TIMER_H__

#include <memory>
#include <atomic>
#include <set>
#include "thread.h"
#include "macro.h"

namespace qff {

class TimerManager;

class Timer final : public std::enable_shared_from_this<Timer> {
friend TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    typedef std::function<void()> CallBackType;

    int cancel();
    int refresh();
    int reset(int ms, bool from_now);
private:
    Timer(int ms, CallBackType cb, bool recurring, TimerManager* manager);
    Timer(int next_time);
private:
    bool m_recurring = false;
    int m_ms;
    int m_next_time;
    CallBackType m_cb;
    TimerManager* m_manager;
private:
    struct Comparator {
        bool operator()(const Timer::ptr lhs, const Timer::ptr rhs) const noexcept;
    };
};

class TimerManager {
friend class Timer;
public:
    NONECOPYABLE(TimerManager);
    typedef RWMutex RWMutexType;

    TimerManager() noexcept;
    virtual ~TimerManager() noexcept;

    Timer::ptr add_timer(int ms, Timer::CallBackType cb, bool recurring = false);
    Timer::ptr add_cond_timer(int ms, Timer::CallBackType cb, std::weak_ptr<void> cond, bool recurring = false);

    int get_next_time() noexcept;
    bool has_timer() noexcept;
protected:
    virtual void on_timer_inserted_into_front() noexcept = 0;

    std::vector<Timer::CallBackType> list_expired_cb();
    void timer_manager_stop() noexcept;
private:
    void add_timer(Timer::ptr timer);
    void detect_clock_rollover(int now_ms) noexcept;
private:
    std::atomic<int> m_previous_time = {-1};
    std::atomic<bool> m_tickled = {false};
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    RWMutexType m_mutex;
};

} // namespace qff


#endif