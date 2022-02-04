#include "fiber.h"

#include <atomic>
#include <assert.h>

#include "log.h"
#include "scheduler.h"

namespace qff {
    
static std::atomic<fid_t> s_fiber_id {0};
static std::atomic<size_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;

fid_t Fiber::GetFiberId() noexcept {
    if(LIKELY(t_fiber))
        return t_fiber->m_id;
    return 0;
}

Fiber::ptr Fiber::GetCacheFiber() noexcept {
    Fiber::ptr fiber(new Fiber);
    return fiber;
}

Fiber* Fiber::GetThis() noexcept {
    return t_fiber;
}
 
void Fiber::Init() noexcept {
    if(t_thread_fiber)
        return;
    t_thread_fiber.reset(new Fiber);
    t_fiber = t_thread_fiber.get();
}

void Fiber::YieldToReady() noexcept {
    assert(t_fiber != t_thread_fiber.get());
    t_fiber->m_state = READY;
    t_fiber->swap_out();
}

void Fiber::YieldToHold() noexcept {
    assert(t_fiber != t_thread_fiber.get());
    t_fiber->m_state = HOLD;
    t_fiber->swap_out();
}

size_t Fiber::GetTotalFibers() noexcept {
    return s_fiber_count;
}

Fiber::Fiber() noexcept 
    :m_id(++s_fiber_id)
    ,m_state(EXEC) {
    int rt = ::getcontext(&m_uct);
    assert(!rt);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "getcontext() fatal";
        std::terminate();
    }

    ++s_fiber_count;
}

Fiber::Fiber(CallBackType cb, size_t stacksize, bool use_caller) noexcept 
    :m_id(++s_fiber_id)
    ,m_stack_size(stacksize)
    ,m_cb(cb) {
        
    m_stack = ::malloc(stacksize);

    int rt = ::getcontext(&m_uct);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "getcontext() fatal";
        std::terminate();
    }
    m_uct.uc_link = nullptr;
    m_uct.uc_stack.ss_sp   = m_stack;
    m_uct.uc_stack.ss_size = m_stack_size;

    if(use_caller) {
        makecontext(&m_uct, &CallerMainFunc, 0);
    }
    else
        makecontext(&m_uct, &MainFunc, 0);

    ++s_fiber_count;
}

Fiber::~Fiber() noexcept {
    --s_fiber_count;
    if(LIKELY(m_stack)) {
        ::free(m_stack);
        if(UNLIKELY(m_state == EXEC 
                || m_state == HOLD 
                || m_state == READY)) {
            QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "~Fiber::Fiber fatal";
            std::terminate();
        }
    } else {
        assert(!m_cb);
        assert(m_state == EXEC);

        if(t_fiber == this)
            t_fiber = nullptr;
    }
}

void Fiber::reset(CallBackType cb) noexcept {
    assert(m_stack);
    assert(m_state == TERM || m_state == INIT);

    m_cb = cb;

    int rt = ::getcontext(&m_uct);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "getcontext() fatal";
        std::terminate();
    }
    m_uct.uc_link = nullptr;
    m_uct.uc_stack.ss_sp   = m_stack;
    m_uct.uc_stack.ss_size = m_stack_size;

    makecontext(&m_uct, &MainFunc, 0);

    m_state = INIT;
}

void Fiber::swap_in() noexcept {
    t_fiber = this;
    assert(m_state != EXEC || m_state == TERM);

    t_thread_fiber->m_state = HOLD;
    m_state = EXEC;
    int rt = ::swapcontext(&t_thread_fiber->m_uct, &m_uct);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "swapcontext() fatal. swapIn()";
        std::terminate();
    }
}

void Fiber::swap_out() noexcept {
    t_fiber = t_thread_fiber.get();
    t_fiber->m_state = EXEC;
    int rt = ::swapcontext(&m_uct, &t_thread_fiber->m_uct);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "swapcontext() fatal. swapOut()";
        std::terminate();
    }
}

void Fiber::call() noexcept {
    t_fiber = this;
    assert(m_state != EXEC || m_state == TERM);

    Scheduler::GetCacheFiber()->m_state = HOLD;
    m_state = EXEC;

    int rt = ::swapcontext(&Scheduler::GetCacheFiber()->m_uct, &m_uct);
    if(rt) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "swapcontext() fatal. swapIn()";
        std::terminate();
    }
}

void Fiber::back() noexcept {
    t_fiber = Scheduler::GetCacheFiber();
    t_fiber->m_state = EXEC;
    int rt = ::swapcontext(&m_uct, &Scheduler::GetCacheFiber()->m_uct);
    if(UNLIKELY(rt)) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "swapcontext() fatal. swapOut()";
        std::terminate();
    }
}

void Fiber::MainFunc() noexcept {
    Fiber* cur = t_fiber;
     try {
         cur->m_cb();
         cur->m_cb = nullptr;
         cur->m_state = TERM;
     } catch(const std::exception& e) {
         cur ->m_state = EXCEPT;
         QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Fiber Except: " << e.what();
     } catch(...) {
         cur->m_state = EXCEPT;
         QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Fiber Except";
     }

     cur->swap_out();
     QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "never reach fiber_id=" << cur->m_id;
     std::terminate();
}

void Fiber::CallerMainFunc() noexcept {
    Fiber* cur = t_fiber;
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(const std::exception& e) {
        cur ->m_state = EXCEPT;
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Fiber Except: " << e.what();
    } catch(...) {
        cur->m_state = EXCEPT;
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Fiber Except";
    }

    cur->back();
    QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "never reach fiber_id=" << cur->m_id;
    std::terminate();
}

} // namespace qff
