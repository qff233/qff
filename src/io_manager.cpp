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

#define XX(ctl) \
    case ctl: \
        return os << #ctl;

    switch ((int)op) {
    XX(EPOLL_CTL_ADD);
    XX(EPOLL_CTL_MOD);
    XX(EPOLL_CTL_DEL);
    default:
        return os << (int)op;
    }
#undef XX
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

void __EventContext::clear() noexcept {
    scheduler = nullptr;
    fiber.reset();
    cb = nullptr;
}

__EventContext& __FdContext::get_context(EventType event_type) noexcept {
    switch(event_type) {
    case E_T_READ:
        return read;
    case E_T_WRITE:
        return write;
    default:
        assert("__FdContext::getContext() invalid event");
    }
    std::terminate();
}

void __FdContext::trigger_event(EventType event_type) {
    assert(event_type & event_types);

    event_types = (EventType)(event_types & ~event_type);
    __EventContext& event_context = get_context(event_type);
    if(event_context.cb)
        event_context.scheduler->schedule(event_context.cb);
    else
        event_context.scheduler->schedule(event_context.fiber);
    
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

    this->context_resize(32);
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
}

void IOManager::context_resize(size_t size) {
    m_fd_contexts.resize(size);
    RWMutexType::WriteLock lock(m_mutex);
    for(size_t i = 0; i < m_fd_contexts.size(); ++i) {
        if(m_fd_contexts[i])
            continue;

        m_fd_contexts[i] = new __FdContext;
        m_fd_contexts[i]->fd = i;
    }
}

int IOManager::add_event(int fd, EventType event_type, CallBackType cb) {
    __FdContext* fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    size_t size = m_fd_contexts.size();
    if(size <= fd)
        this->context_resize(fd * 1.5);
    fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    __FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(fd_ctx->event_types & event_type)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "addEvent assert fd=" << fd
            << " event=" << (EPOLL_EVENTS)event_type
            << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->event_types;
    }

    int ep_op = fd_ctx->event_types ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | fd_ctx->event_types | event_type;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(UNLIKELY(rt)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->event_types;
        return -1;
    }

    ++m_pending_event_count;

    fd_ctx->event_types = (EventType)(fd_ctx->event_types | event_type);
    __EventContext& event_ctx = fd_ctx->get_context(event_type);
    if(UNLIKELY(event_ctx.scheduler)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "event_context has been exist";
    }
    event_ctx.scheduler = t_iomanager;
    if(cb)
        event_ctx.cb.swap(cb);
    else {
        event_ctx.fiber.reset(Fiber::GetThis());
    }
    return 0;
}

int IOManager::del_event(int fd, EventType event_type) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    __FdContext* fd_ctx;
    size_t size = m_fd_contexts.size();
    if(size <= fd)
        return -1;
    fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    __FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->event_types & event_type)))
        return false;
    
    EventType new_epoll_types = (EventType)(fd_ctx->event_types & ~event_type);
    int ep_op = new_epoll_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | new_epoll_types;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->event_types;
        return -1;
    }

    --m_pending_event_count;
    fd_ctx->event_types = new_epoll_types;
    __EventContext& event_ctx = fd_ctx->get_context(event_type);
    event_ctx.clear();
    return 0;
}

int IOManager::cancel_event(int fd, EventType event_type) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    size_t size = m_fd_contexts.size();
    if(size <= fd)
        return -1;
    __FdContext* fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    __FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->event_types & event_type)))
        return false;

    EventType new_epoll_types = (EventType)(fd_ctx->event_types & ~event_type);
    int ep_op = new_epoll_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = EPOLLET | new_epoll_types;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->event_types;
        return -1;
    }

    fd_ctx->trigger_event(event_type);
    --m_pending_event_count;
    return 0;
}

int IOManager::cancel_all(int fd) noexcept {
    RWMutexType::ReadLock lock(m_mutex);
    size_t size = m_fd_contexts.size();
    if(size <= fd)
        return -1;
    __FdContext* fd_ctx = m_fd_contexts[fd];
    lock.unlock();

    __FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(UNLIKELY(!(fd_ctx->event_types)))
        return false;
    
    int ep_op = EPOLL_CTL_DEL;
    epoll_event ep_event;
    ::memset(&ep_event, 0, sizeof(::epoll_event));
    ep_event.events = 0;
    ep_event.data.ptr = fd_ctx;

    int rt = ::epoll_ctl(m_epfd, ep_op, fd, &ep_event);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
            << (EpollOpt)ep_op << ", " << fd << ", " << (EPOLL_EVENTS)ep_event.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->event_types;
        return -1;
    }

    if(fd_ctx->event_types & E_T_READ) {
        fd_ctx->trigger_event(E_T_READ);
        --m_pending_event_count;
    }

    if(fd_ctx->event_types & E_T_WRITE) {
        fd_ctx->trigger_event(E_T_WRITE);
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
    int rt = write(m_tickle_fds[1], "T", 1);
    if(UNLIKELY(rt < 0))
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "IOManager::tickle() write error";
}

bool IOManager::stopping() noexcept {
    return true;
}

void IOManager::idle() {
    static const size_t MAX_EVENT_COUNT = 256;
    epoll_event* ep_events = new epoll_event[MAX_EVENT_COUNT];

    int rt = 0;
    while (!m_is_stop) {
        do {
            static const int MAX_TIMEOUT = 5000;
            rt = ::epoll_wait(m_epfd, ep_events, MAX_EVENT_COUNT, MAX_TIMEOUT);
            if(rt < 0 && errno == EINTR)
                continue;
            break;
        }while(true);

        for(size_t i = 0; i < rt; ++i) {
            epoll_event& event = ep_events[i];
            if(event.data.fd == m_tickle_fds[0]) {
                uint8_t dummy[256];
                while(::read(m_tickle_fds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            __FdContext* fd_ctx = (__FdContext*)event.data.ptr;
            __FdContext::MutexType::Lock lock(fd_ctx->mutex);

            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = E_T_NONE;
            if(event.events & EPOLLIN) {
                real_events |= E_T_READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= E_T_WRITE;
            }

            if((fd_ctx->event_types & real_events) == E_T_NONE) 
                continue;
            
            int left_event = (fd_ctx->event_types & ~real_events);
            int ep_op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_event;

            int rt2 = ::epoll_ctl(m_epfd, ep_op, fd_ctx->fd, &event);
            if(rt2) {
                QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollOpt)ep_op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                    << (EPOLL_EVENTS)fd_ctx->event_types;
                continue;
            }

            if(real_events & E_T_READ) {
                fd_ctx->trigger_event(E_T_READ);
                --m_pending_event_count;
            }

            if(fd_ctx->event_types & E_T_WRITE) {
                fd_ctx->trigger_event(E_T_WRITE);
                --m_pending_event_count;
            }
        }

        Fiber::YieldToHold();
    }

    delete[] ep_events;
}

} // namespace qff
