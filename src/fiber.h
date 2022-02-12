#ifndef __QFF_FIBER_H__
#define __QFF_FIBER_H__

#include <string>
#include <ucontext.h>
#include <memory>
#include <functional>

#include "macro.h"

namespace qff {

typedef ulong fid_t;    

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber>{
friend Scheduler;
public:
    NONECOPYABLE(Fiber);
    typedef std::function<void()> CallBackType;
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        READY,
        EXEC,
        TERM,
        EXCEPT
    };

    static fid_t GetFiberId() noexcept;
    static Fiber::ptr GetThis() noexcept;
    static Fiber::ptr GetCacheFiber() noexcept;
    static void Init() noexcept;
    static void YieldToReady() noexcept;
    static void YieldToHold() noexcept;

    static size_t GetTotalFibers() noexcept;

    Fiber(CallBackType cb, size_t stacksize = 1024*1024
                            , bool use_caller = false) noexcept;
    ~Fiber() noexcept;

    void reset(CallBackType cb) noexcept;
    //when it is not the main thread that is used to schedule
    //two functions below would be used
    void swap_in() noexcept;
    void swap_out() noexcept;
    //when it is the main thread that is used to schedule,
    //two functions below would be used
    void call() noexcept;
    void back() noexcept;
private:
    Fiber() noexcept;

    static void MainFunc() noexcept;
    static void CallerMainFunc() noexcept;
private:
    fid_t m_id = 0;
    State m_state = INIT;

    size_t m_stack_size = 0;
    void* m_stack = nullptr;

    ucontext_t m_uct;
    CallBackType m_cb;
};

} // namespace qff


#endif