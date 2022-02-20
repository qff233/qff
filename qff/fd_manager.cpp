#include "fd_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>

#include "hook.h"

namespace qff {
    
FdContext::FdContext(int fd) noexcept
    :fd(fd) {
    recv_timeout = -1;
    send_timeout = -1;

    struct stat fd_stat;
    int rt = ::fstat(fd, &fd_stat);
    if(rt) {
        is_init = false;
        is_socket = false;
    } else {
        is_init = true;
        is_socket = S_ISSOCK(fd_stat.st_mode);
    }

    if(is_socket) {
        int flags = ::fcntl_f(fd, F_GETFL, 0);
        if(!(flags & O_NONBLOCK))
            ::fcntl_f(fd, F_SETFL, flags | O_NONBLOCK);
        sys_non_block = true;
    } else 
        sys_non_block = false;
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdContext::ptr FdManager::add_or_get_fdctx(int fd, bool auto_create) {
    RWMutexType::ReadLock lock(m_mutex);

    if(m_datas.size() <= (size_t)fd) {
        if(!auto_create)
            return nullptr;
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);

        m_datas.resize(m_datas.size() * 1.5);
    }
    
    if(m_datas[fd])
        return m_datas[fd];

    FdContext::ptr ctx = std::make_shared<FdContext>(fd);
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del_fdctx(int fd) noexcept {
    RWMutexType::WriteLock lock(m_mutex);
    if(m_datas.size() <= (size_t)fd)
        return;
    m_datas[fd].reset();
}

} // namespace qff
