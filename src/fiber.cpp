#include "fiber.h"

#include <atomic>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "scheduler.h"
#include "thread.h"

namespace qff {

class Allocater {
public:
    typedef std::shared_ptr<Allocater> ptr;
    typedef RWMutex RWMutexType;

    struct Node {
        void* ptr = nullptr;
        std::atomic<bool> is_used = {false};
        std::atomic<bool> is_create_base = {false};
        Node(){
        }
        Node(void* ptr, bool is_used, bool is_create_base)
            :ptr(ptr)
            ,is_used(is_used)
            ,is_create_base(is_create_base) {
        }
        Node(const Node& other) {
            ptr = other.ptr;
            is_used.store(other.is_used);
            is_create_base.store(other.is_create_base);
        }
    };

    Allocater(size_t amount, size_t stack_size) 
        :m_stack_size(stack_size) {

        this->resize(amount);
    }

    ~Allocater() noexcept {
        for(auto it = m_memorys.begin(); it != m_memorys.end(); ++it) {
            if(it->is_create_base)
                ::free(it->ptr);
        }
    }

    //         ptr    pos
    std::pair<void*, size_t> malloc() {
        if(m_now_pos >= m_memorys.size() || m_memorys[m_now_pos].is_used) {
            static constexpr size_t nope = static_cast<size_t>(-1);
            static std::atomic<size_t> search_count = {0};
            bool has_resized = false;
            size_t old_pos = m_memorys.size();
            
            if(search_count == 3) {
                this->resize(m_memorys.size() * 1.5);
                search_count = 0;
                has_resized = true;
            }

            size_t pos = nope;
            if(!has_resized) {
                ++search_count;
                for(size_t i = 0; i < m_memorys.size(); ++i) {
                    if(!m_memorys[i].is_used) {
                        pos = i;
                        QFF_LOG_DEBUG(QFF_LOG_SYSTEM) << "for() break" << "     pos=" << pos;
                        break;
                    }
                }
            }

            if(pos == nope) {
                m_now_pos = old_pos;
                if(!has_resized) {
                    this->resize(m_memorys.size() * 1.5);
                    search_count = 0;
                }
            } else {
                m_now_pos = pos;
            }
        }
        
        size_t result_pos = m_now_pos++;
        m_memorys[result_pos].is_used = true;
        QFF_LOG_DEBUG(QFF_LOG_SYSTEM) << m_memorys[result_pos].ptr << "-----" << result_pos;
        return {m_memorys[result_pos].ptr, result_pos};
    }

    void free(size_t pos) noexcept {
        RWMutexType::WriteLock lock(m_mutex);
        if(pos >= m_memorys.size())
            return;
        m_memorys[pos].is_used = false;
    }

    void resize(size_t amount) {
        RWMutexType::WriteLock lock(m_mutex);
        size_t old_size = m_memorys.size();
        m_memorys.resize(amount);
        m_memorys[old_size].is_create_base = true;

        void *ps = ::malloc((amount - old_size) * m_stack_size);
        size_t p_pos = 0;
        for(; old_size < amount; ++old_size) {
            void* p = ((char*)ps + p_pos);
            m_memorys[old_size].ptr = p;
            p_pos += m_stack_size;
        }
    }
private:
    size_t m_stack_size;
    std::atomic<size_t> m_now_pos = 0;
    std::vector<Node> m_memorys;
    RWMutexType m_mutex;
};

static Allocater::ptr s_allocater = nullptr;

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

Fiber::ptr Fiber::GetThis() noexcept {
    return t_fiber->shared_from_this();
}
 
void Fiber::Init(size_t amount, size_t stack_base_size) {
    s_allocater = std::make_shared<Allocater>(amount, stack_base_size);
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
        
    auto[stack, alloc_pos] = s_allocater->malloc();
    m_stack = stack;
    m_alloc_pos = alloc_pos;

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
        s_allocater->free(m_alloc_pos);
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
