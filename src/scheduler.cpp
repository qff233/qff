#include "scheduler.h"

#include <assert.h>

#include "utils.h"
#include "log.h"

namespace qff {
    
static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_cache_fiber = nullptr;

void FiberAndThread::clear() {
    fiber.reset();
    thread_id = -1;
}

FiberAndThread::FiberAndThread() noexcept {
}

FiberAndThread::FiberAndThread(Fiber::ptr fib, ::pid_t id) noexcept 
    :fiber(fib)
    ,thread_id(id) {
}

FiberAndThread::FiberAndThread(CallBackType cb, ::pid_t id) noexcept 
    :thread_id(id) {
    fiber = std::make_shared<Fiber>(cb);
}

FiberAndThread::FiberAndThread(const FiberAndThread& fat) noexcept {
    fiber = fat.fiber;
    thread_id = fat.thread_id;
}

FiberAndThread& FiberAndThread::operator=(const FiberAndThread& fat) {
    fiber = fat.fiber;
    thread_id = fat.thread_id;
    return *this;
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetCacheFiber() noexcept {
    return t_cache_fiber;
}

Scheduler::Scheduler(size_t threads, const std::string& name, bool use_caller) noexcept 
    :m_name(name) {
    
    if(use_caller) {
        Fiber::Init();
        --threads;

        assert(t_scheduler == nullptr);
        t_scheduler = this;

        auto func = std::bind(&Scheduler::run, this);
        m_root_fiber 
            = std::make_shared<Fiber>(func, 1024*1024, true);
        Thread::SetName(m_name);

        m_cache_fiber = Fiber::GetCacheFiber();
        t_cache_fiber = m_cache_fiber.get();
        m_root_thread_id = GetThreadId();
        m_thread_ids.push_back(m_root_thread_id);
    } else {
        m_root_thread_id = -1;
    }
    m_thread_count = threads;
}

Scheduler::~Scheduler() noexcept {
    assert(m_is_stop);
    if(t_scheduler == this) 
        t_scheduler = nullptr;
}

void Scheduler::start() noexcept {
    assert(m_is_stop);
    assert(!m_stop_sign);
    m_is_stop = false;

    assert(m_thread_pool.empty());
    m_thread_pool.resize(m_thread_count);

    auto func = std::bind(&Scheduler::run, this);
    for(size_t i = 0; i < m_thread_count; ++i) {
        Thread::ptr thread 
            = std::make_shared<Thread>(func, m_name+'_'+std::to_string(i));
        m_thread_pool[i] = thread;
        pid_t id = thread->get_id();
        m_thread_ids.push_back(id);
    }
}

void Scheduler::stop() noexcept {
    if(m_is_stop)
        return;
    
    MutexType::Lock lock(m_mutex);
    m_stop_sign = true;
    lock.unlock();

    if(m_root_fiber) 
        m_root_fiber->call();

    for(auto i : m_thread_pool) {
        i->join();
    }

    m_is_stop = true;
}

void Scheduler::schedule(Fiber::ptr fiber, pid_t thread_id) noexcept {
    MutexType::Lock lock(m_mutex);
    bool need_tickle = m_fiber_list.empty();
    m_fiber_list.emplace_back(fiber, thread_id);
    lock.unlock();
    if(need_tickle) 
        this->tickle();
}

void Scheduler::schedule(CallBackType cb, pid_t thread_id) noexcept {
    MutexType::Lock lock(m_mutex);
    bool need_tickle = m_fiber_list.empty();
    m_fiber_list.emplace_back(cb, thread_id);
    lock.unlock();
    if(need_tickle) 
        this->tickle();
}

void Scheduler::schedule(const std::vector<Fiber::ptr>& fibs) noexcept {
    MutexType::Lock lock(m_mutex);
    bool need_tickle = m_fiber_list.empty();
    for(auto i : fibs) {
        m_fiber_list.emplace_back(i);
    }
    lock.unlock();
    if(need_tickle) 
        this->tickle();
}

void Scheduler::schedule(const std::vector<CallBackType>& cbs) noexcept {
    MutexType::Lock lock(m_mutex);
    bool need_tickle = m_fiber_list.empty();
    for(auto i : cbs) {
        m_fiber_list.emplace_back(i);
    }
    if(need_tickle) 
        this->tickle();
}

void Scheduler::tickle() noexcept {
    QFF_LOG_INFO(QFF_LOG_SYSTEM) << "scheduler::tickle()";
}

bool Scheduler::stopping() noexcept {
    QFF_LOG_INFO(QFF_LOG_SYSTEM) << "scheduler::stopping()";
    return true;
}

void Scheduler::idle() noexcept {
    while(!m_stop_sign) {
        QFF_LOG_INFO(QFF_LOG_SYSTEM) << "scheduler::idle()";
        Fiber::YieldToHold();
    }
}

void Scheduler::run() noexcept {
    t_scheduler = this;
    Fiber::Init();

    auto func = std::bind(&Scheduler::idle, this);
    Fiber::ptr idle_fiber = std::make_shared<Fiber>(func);
    FiberAndThread ft;
    bool tick_me;
    bool is_active;
    MutexType::Lock lock(m_mutex, false);
    while(true) {
        ft.clear();
        tick_me = false;
        is_active =false;

        lock.lock();
        auto it = m_fiber_list.begin();
        for(;it != m_fiber_list.end(); ++it) {
            pid_t id = it->thread_id;
            if(id != -1 && id != GetThreadId()) {
                tick_me = true;
                continue;
            }

            assert(it->fiber);
            if(it->fiber->m_state == Fiber::EXEC)
                continue;
            
            ft = *it;
            m_fiber_list.erase(it++);
            break;
        }
        tick_me |= it != m_fiber_list.end();
        lock.unlock();

        if(ft.fiber) {
            ++m_active_thread_count;
            is_active = true;
        }

        if(tick_me)
            tickle();

        if(ft.fiber && ft.fiber->m_state != Fiber::TERM 
            && ft.fiber->m_state != Fiber::EXCEPT) {
            ft.fiber->swap_in();
            --m_active_thread_count;
            if(ft.fiber->m_state == Fiber::READY)
                this->schedule(ft.fiber);
        } else {
            if(is_active) {
                --m_active_thread_count;
                continue;
            }
            if(m_stop_sign && idle_fiber->m_state == Fiber::TERM)
                break;

            ++m_idle_thread_count;
            idle_fiber->swap_in();
            --m_idle_thread_count;
        }
    }
} //Scheduler::run()

bool Scheduler::has_idle_threads() {
    return m_idle_thread_count > 0;
}

} // namespace qff
