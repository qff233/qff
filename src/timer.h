#ifndef __QFF__TIMER_H__
#define __QFF__TIMER_H__

#include <memory>
#include <set>
#include "thread.h"
#include "macro.h"

namespace qff {

class TimerManager;

class Timer {
friend TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    typedef std::function<void()> CallBackType;

    bool cancel();
    bool refresh();
    bool reset(int ms, bool from_now);
private:
    Timer(int ms, CallBackType cb, bool recurring, TimerManager* manager);
private:
    bool m_recurring = false;
    int m_ms;
    int m_next;
    CallBackType m_cb;
    TimerManager* m_manager;
private:
    struct Comparator {
        bool operator()(const Timer::ptr lhs, const Timer::ptr rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    NONECOPYABLE(TimerManager);
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

protected:
    virtual void on_timer_inserted_into_front() = 0;
private:
    int m_previous_time = -1;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    RWMutexType m_mutex;
};

} // namespace qff


#endif