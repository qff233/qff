#ifndef __IO_MANAGER_H__
#define __IO_MANAGER_H__

#include <memory>

#include "scheduler.h"

namespace qff {

enum EventType {
    E_T_NONE  = 0x0,
    E_T_READ  = 0x1,
    E_T_WRITE = 0x4
};

struct __EventContext {
    typedef std::function<void()> CallBackType;

    Scheduler* scheduler = nullptr;
    Fiber::ptr fiber;
    CallBackType cb;

    void clear() noexcept;
};

struct __FdContext {
    typedef Mutex MutexType;

    __EventContext read;
    __EventContext write;
    int fd = 0;
    EventType event_types = E_T_NONE;
    MutexType mutex;
    
    __EventContext& get_context(EventType event_type) noexcept;
    void trigger_event(EventType event_type);
};

class IOManager final : public Scheduler {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef std::function<void()> CallBackType;
    typedef RWMutex RWMutexType;

    static IOManager* GetThis();

    IOManager(size_t thread_count = 1, const std::string& name = "", bool use_caller = true);
    ~IOManager() noexcept;

    int add_event(int fd, EventType event_type, CallBackType cb = nullptr);
    int del_event(int fd, EventType event_type) noexcept;

    int cancel_event(int fd, EventType event_type) noexcept;
    int cancel_all(int fd) noexcept;
protected:
    void context_resize(size_t size);

    void init() override;
    void tickle() noexcept override;
    bool stopping() noexcept override;
    void idle() override;
private:
    int m_epfd = -1;
    int m_tickle_fds[2];
    std::atomic<size_t> m_pending_event_count = {0};
    RWMutexType m_mutex;
    std::vector<__FdContext*> m_fd_contexts;
};

} // namespace qff


#endif