#ifndef __IO_MANAGER_H__
#define __IO_MANAGER_H__

#include <memory>
#include <variant>

#include "scheduler.h"
#include "timer.h"
#include "hook.h"

namespace qff {

class IOManager final : public Scheduler, public TimerManager {
friend void SetSleepySign(qff::IOManager* iom, bool flag);
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef std::function<void()> CallBackType;
    typedef RWMutex RWMutexType;

    enum EventType {
        NONE  = 0x0,
        READ  = 0x1,
        WRITE = 0x4,
    };
private:
    struct EventContext {
        typedef std::function<void()> CallBackType;
        typedef std::variant<Fiber::ptr, CallBackType> ContextType;

        Scheduler* scheduler = nullptr;
        ContextType fiber_or_func;

        void clear() noexcept;
    };

    struct FdContext {
        typedef Mutex MutexType;

        EventContext read;
        EventContext write;

        int fd = 0;
        EventType events = NONE;

        MutexType mutex;

        EventContext& get_context(EventType event) noexcept;
        void trigger_event(EventType event);
    };
public:
    static IOManager* GetThis();

    IOManager(size_t thread_count = 1, const std::string& name = "", bool use_caller = true);
    ~IOManager() noexcept;

    int add_event(int fd, EventType event, CallBackType cb = nullptr) noexcept;
    int del_event(int fd, EventType event) noexcept;

    int cancel_event(int fd, EventType event) noexcept;
    int cancel_all(int fd) noexcept;
private:
    void contexts_resize(size_t size) noexcept;
protected:
    void init() override;
    void tickle() noexcept override;
    bool stopping() noexcept override;
    void idle() override;

    void on_timer_inserted_into_front() noexcept override;
private:
    int m_epfd = -1;
    int m_tickle_fds[2];
    std::atomic<size_t> m_pending_event_count = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fd_contexts;
};

} // namespace qff


#endif