#include "hook.h"

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdarg.h>

#include "log.h"
#include "io_manager.h"
#include "fd_manager.h"
#include "macro.h"

namespace qff {

static thread_local bool t_hook_enable = false;
static int s_connect_timeout = -1;

#define HOOK_FUN(XX)    \
    XX(sleep)           \
    XX(usleep)          \
    XX(nanosleep)       \
    XX(socket)          \
    XX(connect)         \
    XX(accept)          \
    XX(read)            \
    XX(readv)           \
    XX(recv)            \
    XX(recvfrom)        \
    XX(recvmsg)         \
    XX(write)           \
    XX(writev)          \
    XX(send)            \
    XX(sendto)          \
    XX(sendmsg)         \
    XX(close)           \
    XX(fcntl)           \
    XX(ioctl)           \
    XX(getsockopt)      \
    XX(setsockopt)

bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

void set_connect_timeout(int ms) {
    s_connect_timeout = ms;
}

struct HookIniter {
    HookIniter() {
        #define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX)
        #undef XX
    }
};
static HookIniter s_hook_initer;


void SetSleepySign(qff::IOManager* iom, bool flag) {
    iom->m_sleep_sign = flag;
}

} //namespace qff

struct timer_cond {
    int cancelled = 0;
};

template<class OriginFun, typename ... Args>
static ssize_t do_io(int fd, OriginFun fun, std::string_view hook_fun_name,
        qff::IOManager::EventType event, int timeout_so, Args&&... args) {
    if(!qff::t_hook_enable)
        return fun(fd, std::forward<Args>(args)...);
    
    //QFF_LOG_DEBUG(QFF_LOG_SYSTEM) << hook_fun_name << " is hooked";

    qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(fd);

    if(!ctx)
        return fun(fd, std::forward<Args>(args)...);

    if(!ctx->is_init) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->is_socket || !ctx->sys_non_block)
        return fun(fd, std::forward<Args>(args)...);


    int timeout = timeout_so == SO_RCVTIMEO ? 
                    ctx->recv_timeout : ctx->send_timeout;

    std::shared_ptr<timer_cond> t_cond = std::make_shared<timer_cond>();
    ssize_t result = -1;

    do {
        result = fun(fd, std::forward<Args>(args)...);
        while(result == -1 && errno == EINTR) {
            result = fun(fd, std::forward<Args>(args)...);
        }
        if(result == -1 && errno == EAGAIN) {
            qff::IOManager* iom = qff::IOManager::GetThis();
            qff::Timer::ptr timer;

            if(timeout != -1)
                timer = iom->add_cond_timer(timeout, [&t_cond, fd, iom, event]{
                    t_cond->cancelled = ETIMEDOUT;
                    iom->cancel_event(fd, event);
                }, t_cond);

            int rt = iom->add_event(fd, event);
            if(UNLIKELY(rt)) {
                QFF_LOG_ERROR(QFF_LOG_SYSTEM) << hook_fun_name << " addEvent("
                    << fd << ", " << event << ")";
                timer->cancel();
                return -1;
            }

            qff::Fiber::YieldToHold(); // One probability is timeout,and another is event being triggered.

            if(t_cond->cancelled) {     //this is timeout operation.
                errno = t_cond->cancelled;
                return -1;
            }

            if(timer)
                timer->cancel();

            continue;                   //successly. turn back "do" to run the function again.
        }
    } while(false);

    return result;
}

extern "C" {

#define XX(name) name##_fun name##_f = nullptr;
    HOOK_FUN(XX)
#undef XX



unsigned int sleep(unsigned int seconds) {
    if(!qff::t_hook_enable)
        return sleep_f(seconds);
    
    qff::Fiber::ptr fiber = qff::Fiber::GetThis();
    qff::IOManager* iom = qff::IOManager::GetThis();

    qff::SetSleepySign(iom, true);
    iom->add_timer(seconds*1000, [iom, fiber]{
        iom->schedule(fiber);
        qff::SetSleepySign(iom, false);
    });
    qff::Fiber::YieldToHold();
    return 0;
}
            
int usleep(useconds_t usec) {
    if(!qff::t_hook_enable)
        return usleep_f(usec);
    
    qff::Fiber::ptr fiber = qff::Fiber::GetThis();
    qff::IOManager* iom = qff::IOManager::GetThis();

    qff::SetSleepySign(iom, true);
    iom->add_timer(usec/1000, [iom, fiber]{
        iom->schedule(fiber);
        qff::SetSleepySign(iom, false);
    });
    qff::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!qff::t_hook_enable)
        return nanosleep_f(req, rem);

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    qff::Fiber::ptr fiber = qff::Fiber::GetThis();
    qff::IOManager* iom = qff::IOManager::GetThis();

    qff::SetSleepySign(iom, true);
    iom->add_timer(timeout_ms, [iom, fiber]{
        iom->schedule(fiber);
        qff::SetSleepySign(iom, false);
    });
    qff::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if(!qff::t_hook_enable)
        return socket_f(domain, type, protocol);

    int fd = socket_f(domain, type, protocol);
    if(fd == -1)
        return -1;

    qff::FdMgr::Get()->add_or_get_fdctx(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, int timeout_ms) {
    if(!qff::t_hook_enable)
        return connect_f(fd, addr, addrlen);

    qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(fd);
    if(!ctx || !ctx->is_init) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->is_socket || ctx->sys_non_block)
        return connect_f(fd, addr, addrlen);

    int rt = connect_f(fd, addr, addrlen);
    if(rt == 0)
        return 0;

    if(errno != EINPROGRESS)
        return -1;

    qff::IOManager* iom = qff::IOManager::GetThis();
    qff::Timer::ptr timer;
    std::shared_ptr<timer_cond> t_cond = std::make_shared<timer_cond>();

    if(timeout_ms != -1) {
        timer = iom->add_cond_timer(timeout_ms, [&t_cond, fd, iom]() {
            if(t_cond->cancelled)
                return;
            t_cond->cancelled = ETIMEDOUT;
            iom->cancel_event(fd, qff::IOManager::WRITE);
        }, t_cond);
    }

    rt = iom->add_event(fd, qff::IOManager::WRITE);
    if(UNLIKELY(rt)) {
        if(timer)
            timer->cancel();
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "connect addEvent(" << fd << ", WRITE) error";
    }

    qff::Fiber::YieldToHold();

    if(t_cond->cancelled) {
        errno = t_cond->cancelled;
        return -1;
    }

    if(timer)
        timer->cancel();

    int error = 0;
    socklen_t len = sizeof(int);
    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
        return -1;

    if(error) {
        errno = error;
        return -1;
    } 

    return 0;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, qff::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", qff::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd < 0)
        return -1;

    qff::FdMgr::Get()->add_or_get_fdctx(fd, true);
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", qff::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", qff::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", qff::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", qff::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", qff::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", qff::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", qff::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", qff::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", qff::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", qff::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!qff::t_hook_enable)
        return close_f(fd);

    qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(fd);
    if(ctx) {
        auto iom = qff::IOManager::GetThis();
        if(iom)
            iom->cancel_all(fd);

        qff::FdMgr::Get()->del_fdctx(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(fd);
                if(!ctx || !ctx->is_socket)
                    return fcntl_f(fd, cmd, arg);

                ctx->sys_non_block = arg & O_NONBLOCK;
                return fcntl_f(fd, cmd, arg);
            }
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
        case F_GETFL:
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        return ioctl_f(d, request, arg);
    }

    bool nonblock = *(int*)arg;
    qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(d);
    if(ctx && ctx->is_socket)
        ctx->sys_non_block = nonblock;

    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!qff::t_hook_enable)
        return setsockopt_f(sockfd, level, optname, optval, optlen);

    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            qff::FdContext::ptr ctx = qff::FdMgr::Get()->add_or_get_fdctx(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                if(optname == SO_RCVTIMEO)
                    ctx->recv_timeout = v->tv_sec * 1000 + v->tv_usec / 1000;
                else
                    ctx->send_timeout = v->tv_sec * 1000 + v->tv_usec / 1000;
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}