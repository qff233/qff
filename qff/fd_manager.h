#ifndef __QFF_FD_MANAGER_H__
#define __QFF_FD_MANAGER_H__

#include "thread.h"
#include "io_manager.h"
#include "singleton.h"

namespace qff {

struct FdContext final {
    typedef std::shared_ptr<FdContext> ptr;

    bool is_init: 1;
    bool is_socket: 1;
    bool sys_non_block: 1;

    int fd;

    uint64_t recv_timeout;
    uint64_t send_timeout;

    FdContext(int fd) noexcept;
};

class FdManager final {
public:
    typedef RWMutex RWMutexType;

    FdManager();

    FdContext::ptr add_or_get_fdctx(int fd, bool auto_create = false);
    void del_fdctx(int fd) noexcept;
private:
    RWMutexType m_mutex;
    std::vector<FdContext::ptr> m_datas;
};

using FdMgr = Singleton<FdManager>;

} //namespace

#endif