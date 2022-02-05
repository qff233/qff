#ifndef __QFF_SCHEDULER_H__
#define __QFF_SCHEDULER_H__

#include <sys/types.h>
#include <memory>
#include <vector>
#include <list>
#include <atomic>

#include "macro.h"
#include "thread.h"
#include "fiber.h"

namespace qff {
    
struct __FiberAndThread {
    typedef std::function<void()> CallBackType;
    Fiber::ptr fiber;
    ::pid_t thread_id = -1;

    void clear();

    __FiberAndThread() noexcept;
    __FiberAndThread(Fiber::ptr fib, ::pid_t id = -1) noexcept;
    __FiberAndThread(CallBackType cb, ::pid_t id = -1) noexcept;
    __FiberAndThread(const __FiberAndThread& fat) noexcept;
    __FiberAndThread& operator=(const __FiberAndThread& fat);
};

class Fiber;

class Scheduler {
public:
    friend Fiber;
    NONECOPYABLE(Scheduler);
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    typedef std::function<void()> CallBackType;
    typedef std::list<__FiberAndThread> FListType;
    typedef std::vector<Thread::ptr> TPoolType;

    static Scheduler* GetThis();

    Scheduler(size_t thread_count = 1, const std::string& name="", bool use_caller = true);
    virtual ~Scheduler() noexcept;

    void start();
    void stop() noexcept;

    void schedule(Fiber::ptr fiber, ::pid_t thread_id = -1);
    void schedule(CallBackType cb, ::pid_t thread_id = -1);
    void schedule(const std::vector<Fiber::ptr>& fibs);
    void schedule(const std::vector<CallBackType>& cbs);

    const std::string& get_name() const { return m_name; }
private:
    static Fiber* GetCacheFiber() noexcept;
    void idle_base();
protected:
    virtual void init();
    virtual void tickle() noexcept;
    virtual bool stopping() noexcept;
    virtual void idle();
    void run();

    bool has_idle_threads() noexcept;
private:
    MutexType m_mutex;
    FListType m_fiber_list;
    TPoolType m_thread_pool;
    Fiber::ptr m_root_fiber;
    Fiber::ptr m_cache_fiber;
    std::string m_name;
protected:
    std::vector<::pid_t> m_thread_ids;
    size_t m_thread_count = 0;
    std::atomic<size_t> m_active_thread_count = {0};
    std::atomic<size_t> m_idle_thread_count = {0};
    ::pid_t m_root_thread_id = -1;
    std::atomic<bool> m_stop_sign = {false};
    std::atomic<bool> m_is_stop = {true};
};

} // namespace qff


#endif