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

class Fiber;

class Scheduler {
friend Fiber;
private:
    struct FiberAndThread {
        typedef std::function<void()> CallBackType;
        Fiber::ptr fiber;
        ::pid_t thread_id = -1;

        void clear();

        FiberAndThread() noexcept;
        FiberAndThread(Fiber::ptr fib, ::pid_t id = -1) noexcept;
        FiberAndThread(CallBackType cb, ::pid_t id = -1) noexcept;
        FiberAndThread(const FiberAndThread& fat) noexcept;
        FiberAndThread& operator=(const FiberAndThread& fat);
    };
public:
    NONECOPYABLE(Scheduler);
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;
    typedef std::function<void()> CallBackType;
    
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
    virtual void tickle();
    virtual bool stopping();
    virtual void idle();
    void run();

    bool has_idle_threads() noexcept;
private:
    MutexType m_mutex;
    std::list<FiberAndThread> m_fiber_list;
    //record the fiber that will be run.
    std::vector<Thread::ptr> m_thread_pool;
    Fiber::ptr m_root_fiber;
    Fiber::ptr m_cache_fiber;
    std::string m_name;
protected:
    std::vector<::pid_t> m_thread_ids;
    size_t m_thread_count = 0;
    std::atomic<size_t> m_active_thread_count = {0};
    std::atomic<size_t> m_idle_thread_count = {0};
    ::pid_t m_root_thread_id = -1;
    std::atomic<bool> m_is_stopping = {false};
    std::atomic<bool> m_stop_sign = {false};
    std::atomic<bool> m_sleep_sign = {false};
    std::atomic<bool> m_is_stop = {true};
};

} // namespace qff


#endif