#include "io_manager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

namespace qff {
    
enum EpollOpt {
};

static std::ostream& operator<< (std::ostream& os, const EpollOpt& op) {
    switch(op) {
    case EPOLL_CTL_ADD:
        os << "EPOLL_CTL_ADD";
        return os;
    case EPOLL_CTL_MOD:
        os << "EPOLL_CTL_MOD";
        return os;
    case EPOLL_CTL_DEL:
        os << "EPOLL_CTL_DEL";
        return os;
    default:
        return os << (int)op;
    }
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;

#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }

    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

void IOManager::EventContext::clear() noexcept {
    scheduler = nullptr;
    if(auto p = std::get_if<Fiber::ptr>(&fiber_or_func)) 
        p->reset();
    if(auto p = std::get_if<CallBackType>(&fiber_or_func))
        *p = nullptr;
}

IOManager::EventContext& IOManager::FdContext::get_context(EventType event) noexcept {
    switch(event) {
    case READ:
        return read;
    case WRITE:
        return write;
    default:
        assert("__FdContext::getContext() invalid event");
    }
    std::terminate();
}

void IOManager::FdContext::trigger_event(EventType event) {
    assert(event & events);

    events = (EventType)(events & ~event);
    EventContext& event_context = get_context(event);
    if(auto p1 = std::get_if<CallBackType>(&event_context.fiber_or_func)) {
        event_context.scheduler->schedule(*p1);
    } else {
        auto p2 = std::get_if<Fiber::ptr>(&event_context.fiber_or_func);
        event_context.scheduler->schedule(*p2);
    }
    
    event_context.clear();
}

static thread_local IOManager* t_iomanager = nullptr;

IOManager* IOManager::GetThis() {
    return t_iomanager;
}

IOManager::IOManager(size_t thread_count, const std::string& name, bool use_caller) 
    :Scheduler(thread_count, name, use_caller) {
    m_epfd = ::epoll_create(6666);
    if(m_epfd <= 0) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "epoll_create() fatal";
        std::terminate();
    }

    int rt = ::pipe(m_tickle_fds);
    if(rt) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "pipe(m_tickle_fds) fatal";
        std::terminate();
    }

    ::epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLIN | EPOLLET;
    ep_event.data.fd = m_tickle_fds[0];

    rt = ::fcntl(m_tickle_fds[0], F_SETFL, O_NONBLOCK);
    if(rt) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "fcntl(m_tickle_fds[0], F_SETFL, O_NONBLOCK) fatal\n"
            << "errno:" << errno << "\nerrno_str:" << strerror(errno);
        std::terminate();
    }

    rt = ::epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickle_fds[0], &ep_event);
    if(rt) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickle_fds[0], &ep_event) fatal\n"
            << "errno:" << errno << "\nerrno_str:" << strerror(errno);
        std::terminate();
    }

    this->contexts_resize(32);
    this->start();
}

IOManager::~IOManager() noexcept {
    this->stop();
    ::close(m_epfd);
    ::close(m_tickle_fds[0]);
    ::close(m_tickle_fds[1]);

    for(size_t i = 0; i < m_fd_contexts.size(); ++i) {
        if(!m_fd_contexts[i])
            continue;
        delete m_fd_contexts[i];
    }
    if(t_iomanager == this) 
        t_iomanager = nullptr;
}

void IOManager::contexts_resize(size_t size) noexcept {
    size_t i = m_fd_contexts.size();
    m_fd_contexts.resize(size);
    RWMutexType::WriteLock lock(m_mutex);
    try {
        for(;i < m_fd_contexts.size(); ++i) {
            m_fd_contexts[i] = new FdContext;
            m_fd_contexts[i]->fd = i;
        }
    } catch (const std::exception& e) {
        QFF_LOG_FATAL(QFF_LOG_SYSTEM) << "In IOManager::context_resize," << e.what();
        std::terminate();
    }
}

int IOManager::add_event(int fd, EventType event, CallBackType cb) noexcept {
    FdContext* fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    if(m_fd_contexts.size() <= (size_t)fd)
        this->contexts_resize(fd * 1.5);
    fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(fd_ctx->events & event)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "addEvent assert fd=" << fd
            << " event=" << (EPOLL_EVENTS)event
            << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
    }

    int ep_op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | fd_ctx->events | event;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(UNLIKELY(rt)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pending_event_count;

    fd_ctx->events = (EventType)(fd_ctx->events | event);
    EventContext& event_ctx = fd_ctx->get_context(event);
    if(UNLIKELY(event_ctx.scheduler)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "event_context has been exist";
    }
    event_ctx.scheduler = t_iomanager;
    
    if(cb)
        event_ctx.fiber_or_func = cb;
    else {
        event_ctx.fiber_or_func = Fiber::ptr(Fiber::GetThis());
    }
    return 0;
}

int IOManager::del_event(int fd, EventType event) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    FdContext* fd_ctx;
    if(m_fd_contexts.size() <= (size_t)fd)
        return -1;
    fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->events & event)))
        return false;
    
    EventType new_epoll_types = (EventType)(fd_ctx->events & ~event);
    int ep_op = new_epoll_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ::epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | new_epoll_types;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    --m_pending_event_count;
    fd_ctx->events = new_epoll_types;
    EventContext& event_ctx = fd_ctx->get_context(event);
    event_ctx.clear();
    return 0;
}

int IOManager::cancel_event(int fd, EventType event) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    if(m_fd_contexts.size() <= (size_t)fd)
        return -1;
    FdContext* fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->events & event)))
        return false;

    EventType new_epoll_types = (EventType)(fd_ctx->events & ~event);
    int ep_op = new_epoll_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ::epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | new_epoll_types;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    fd_ctx->trigger_event(event);
    --m_pending_event_count;
    return 0;
}

int IOManager::cancel_all(int fd) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    if(m_fd_contexts.size() <= (size_t)fd)
        return -1;
    FdContext* fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->events)))
        return false;
    
    int ep_op = EPOLL_CTL_DEL;
    ::epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = 0;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->trigger_event(READ);
        --m_pending_event_count;
    }

    if(fd_ctx->events & WRITE) {
        fd_ctx->trigger_event(WRITE);
        --m_pending_event_count;
    }

    return 0;
}

void IOManager::init() {
    t_iomanager = this;
}

void IOManager::tickle() noexcept {
    if(!has_idle_threads())
        return;
    int rt = ::write(m_tickle_fds[1], "T", 1);
    if(UNLIKELY(rt < 0))
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "IOManager::tickle() write error";
}

bool IOManager::stopping() noexcept {
    return m_pending_event_count == 0;
}

void IOManager::idle() {
    static const size_t MAX_EVENT_COUNT = 256;
    epoll_event* ep_events = new epoll_event[MAX_EVENT_COUNT];

    int rt = 0;
    while (!m_is_stopping) {
        do {
            static const int MAX_TIMEOUT = 5000;
            rt = ::epoll_wait(m_epfd, ep_events, MAX_EVENT_COUNT, MAX_TIMEOUT);
            if(rt < 0 && errno == EINTR)
                continue;
            break;
        }while(true);

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = ep_events[i];
            if(event.data.fd == m_tickle_fds[0]) {
                uint8_t dummy[256];
                while(::read(m_tickle_fds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);

            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE) 
                continue;
            
            int left_event = (fd_ctx->events & ~real_events);
            int ep_op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_event;

            int rt2 = ::epoll_ctl(m_epfd, ep_op, fd_ctx->fd, &event);
            if(rt2) {
                QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollOpt)ep_op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                    << (EPOLL_EVENTS)fd_ctx->events;
                continue;
            }

            if(real_events & READ) {
                fd_ctx->trigger_event(READ);
                --m_pending_event_count;
            }

            if(fd_ctx->events & WRITE) {
                fd_ctx->trigger_event(WRITE);
                --m_pending_event_count;
            }
        }

        Fiber::YieldToHold();
    }

    delete[] ep_events;
}

} // namespace qff
