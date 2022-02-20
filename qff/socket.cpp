#include "socket.h"
#include "fd_manager.h"
#include "log.h"
#include "utils.h"

#include <netinet/tcp.h>

namespace qff {
    
Socket::Socket(int family, int type, int protocol) 
    :m_sock(-1) 
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol) 
    ,m_is_connected(false) {
}

Socket::~Socket() noexcept {
    this->close();
}

int Socket::get_send_timeout() {
    FdContext::ptr ctx =  FdMgr::Get()->add_or_get_fdctx(m_sock);
    if(ctx)
        return ctx->send_timeout;
    return -1;
}

void Socket::set_send_timeout(int v) {
    timeval tv{v/1000, v%1000*1000};
    this->set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int Socket::get_recv_timeout() {
    FdContext::ptr ctx =  FdMgr::Get()->add_or_get_fdctx(m_sock);
    if(ctx)
        return ctx->recv_timeout;
    return -1;
}

void Socket::set_recv_timeout(int v) {
    timeval tv{v/1000, v%1000*1000};
    this->set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

int Socket::get_option(int level, int option, void* result, socklen_t* len) {
    int rt = ::getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "get_option sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << ::strerror(errno);
        return -1;
    }
    return 0;
}

int Socket::set_option(int level, int option, const void* opt, socklen_t len) {
    int rt = ::setsockopt(m_sock, level, option, opt, len);
    if(rt) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "get_option sock=" << m_sock
            << " level=" << level << " option=" << option
            << " errno=" << errno << " errstr=" << ::strerror(errno);
        return -1;
    }
    return 0;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock = std::make_shared<Socket>(m_family, m_type, m_protocol);
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "accept(" << m_sock << ") errno="
            << errno << " errstr=" << ::strerror(errno);
        return nullptr;
    }
    if(sock->create_sock_from_sockfd(newsock)) {
        return nullptr;
    }
    return sock;
}

int Socket::bind(const Address::ptr addr) {
    if(!is_valid()) {
        this->create_sock();
        if(UNLIKELY(!is_valid())) {
            return -1;
        }
    }

    if(UNLIKELY(addr->get_family() != m_family)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "bind sock.family("
            << m_family << ") addr.family(" << addr->get_family()
            << ") not equal, addr=" << addr->to_string();
        return -1;
    }

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        Socket::ptr sock = std::make_shared<Socket>(UNIX, TCP);
        if(sock->connect(uaddr)) {
            return -1;
        } else {
            qff::FSUtils::UnLink(uaddr->get_path(), true);
        }
    }

    if(::bind(m_sock, addr->get_addr(), addr->get_addr_len())) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "bind error errrno=" << errno
            << " errstr=" << strerror(errno);
        return -1;
    }
    this->get_local_address();
    return 0;
}

int Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remote_address = addr;
    if(!is_valid()) {
        this->create_sock();
        if(UNLIKELY(!is_valid())) {
            return -1;
        }
    }

    if(UNLIKELY(addr->get_family() != m_family)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "connect sock.family("
            << m_family << ") addr.family(" << addr->get_family()
            << ") not equal, addr=" << addr->to_string();
        return -1;
    }

    if(timeout_ms == (uint64_t)-1) {
        int rt = ::connect(m_sock, addr->get_addr(), addr->get_addr_len());
        if(rt) {
            QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "sock=" << m_sock << " connect(" << *addr
                << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return -1;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->get_addr(), addr->get_addr_len(), timeout_ms)) {
            QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "sock=" << m_sock << " connect(" << addr->to_string()
                << ") timeout=" << timeout_ms << " error errno="
                << errno << " errstr=" << strerror(errno);
            close();
            return -1;
        }
    }
    m_is_connected = true;
    //get_local_address();
    return 0;
}

int Socket::reconnect(uint64_t timeout_ms) {
    if(!m_remote_address) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "reconnect m_remoteAddress is null";
        return -1;
    }
    m_local_address.reset();
    return connect(m_remote_address, timeout_ms);
}

int Socket::listen(int backlog) {
    if(!is_valid()) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "listen error sock=-1";
        return false;
    }
    if(::listen(m_sock, backlog)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "listen error errno=" << errno
            << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

int Socket::close() {
    if(!m_is_connected && m_sock == -1) 
        return 0;
    m_is_connected = false;
    if(m_sock != -1) {
        int rt = ::close(m_sock);
        m_sock = -1;
        if(rt)
            return -1;
    }
    return 0;
}

int Socket::send(const void* buffer, size_t length, int flags) {
    if(is_connected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec* buffers, size_t length, int flags) {
    if(is_connected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::send_to(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(is_connected()) {
        return ::sendto(m_sock, buffer, length, flags, to->get_addr(), to->get_addr_len());
    }
    return -1;
}

int Socket::send_to(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(is_connected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = (void*)to->get_addr();
        msg.msg_namelen = to->get_addr_len();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if(is_connected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec* buffers, size_t length, int flags) {
    if(is_connected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv_from(void* buffer, size_t length, Address::ptr from, int flags) {
    if(is_connected()) {
        socklen_t len = from->get_addr_len();
        return ::recvfrom(m_sock, buffer, length, flags, (sockaddr*)from->get_addr(), &len);
    }
    return -1;
}

int Socket::recv_from(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(is_connected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = (void*)from->get_addr();
        msg.msg_namelen = from->get_addr_len();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::get_remote_address() {
    if(m_remote_address) {
        return m_remote_address;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
            break;
        default:
            result = std::make_shared<UnknownAddress>(m_family);
            break;
    }
    socklen_t addrlen = result->get_addr_len();
    if(getpeername(m_sock, (sockaddr*)result->get_addr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->set_addr_len(addrlen);
    }
    m_remote_address = result;
    return m_remote_address;
}

Address::ptr Socket::get_local_address() {
    if(m_local_address) {
        return m_local_address;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
            break;
        default:
            result = std::make_shared<UnknownAddress>(m_family);
            break;
    }
    socklen_t addrlen = result->get_addr_len();
    if(getsockname(m_sock, (sockaddr*)result->get_addr(), &addrlen)) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "getsockname error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return std::make_shared<UnknownAddress>(m_family);
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->set_addr_len(addrlen);
    }
    m_local_address = result;
    return m_local_address;
}

bool Socket::is_valid() const {
    return m_sock != -1;
}

int Socket::get_error() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!get_option(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_is_connected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_local_address) {
        os << " local_address=" << m_local_address->to_string();
    }
    if(m_remote_address) {
        os << " remote_address=" << m_remote_address->to_string();
    }
    os << "]";
    return os;
}

std::string Socket::to_string() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

int Socket::cancel_read() {
    return IOManager::GetThis()->cancel_event(m_sock, qff::IOManager::READ);
}

int Socket::cancel_write() {
    return IOManager::GetThis()->cancel_event(m_sock, qff::IOManager::WRITE);
}

int Socket::cancel_accept() {
    return IOManager::GetThis()->cancel_event(m_sock, qff::IOManager::READ);
}

int Socket::cancel_all() {
    return IOManager::GetThis()->cancel_all(m_sock);
}

void Socket::init_sock_opt() {
    int val = 1;
    set_option(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM)
        set_option(IPPROTO_TCP, TCP_NODELAY, val);
}

void Socket::create_sock() {
    m_sock = ::socket(m_family, m_type, m_protocol);
    if(LIKELY(m_sock != -1)) {
        this->init_sock_opt();
    } else {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
    }
}

int Socket::create_sock_from_sockfd(int sock) {
    FdContext::ptr ctx = FdMgr::Get()->add_or_get_fdctx(sock);
    if(ctx && ctx->is_socket && ctx->is_init) {
        m_sock = sock;
        m_is_connected = true;
        this->init_sock_opt();
        //get_local_address();
        //get_remote_address();
        return 0;
    }
    return -1;
}

} // namespace qff 
