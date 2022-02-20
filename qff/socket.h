#ifndef __QFF_SOCKET_H__
#define __QFF_SOCKET_H__

#include <memory>
#include <sys/types.h>
#include <sys/socket.h>

#include "address.h"

namespace qff {
    
class Socket {
public:
    typedef std::shared_ptr<Socket> ptr;

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };
    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX
    };

    Socket(int family, int type, int protocol = 0);
    virtual ~Socket() noexcept;

    int get_send_timeout();
    void set_send_timeout(int v);
    int get_recv_timeout();
    void set_recv_timeout(int v);

    int get_option(int level, int option, void* result, socklen_t* len);
    template<class T>
    int get_option(int level, int option, T& result) {
        socklen_t length = sizeof(T);
        return get_option(level, option, &result, &length);
    }
    int set_option(int level, int option, const void* opt, socklen_t len);
    template<class T>
    int set_option(int level, int option, const T& opt) {
        return set_option(level, option, &opt, sizeof(T));
    }

    virtual Socket::ptr accept();
    virtual int bind(const Address::ptr addr);
    virtual int connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    virtual int reconnect(uint64_t timeout_ms = -1);
    virtual int listen(int backlog = SOMAXCONN);
    virtual int close();

    virtual int send(const void* buffer, size_t length, int flags = 0);
    virtual int send(const iovec* buffers, size_t length, int flags = 0);
    virtual int send_to(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int send_to(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);
    virtual int recv(void* buffer, size_t length, int flags = 0);
    virtual int recv(iovec* buffers, size_t length, int flags = 0);
    virtual int recv_from(void* buffer, size_t length, Address::ptr from, int flags = 0);
    virtual int recv_from(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    Address::ptr get_remote_address();
    Address::ptr get_local_address();

    int get_family() const { return m_family;}
    int get_type() const { return m_type;}
    int get_protocol() const { return m_protocol;}
    bool is_connected() const { return m_is_connected;}
    bool is_valid() const;
    int get_error();

    virtual std::ostream& dump(std::ostream& os) const;
    virtual std::string to_string() const;
    int get_socket() const { return m_sock;}

    int cancel_read();
    int cancel_write();
    int cancel_accept();
    int cancel_all();
protected:
    void init_sock_opt();
    void create_sock();
    virtual int create_sock_from_sockfd(int sock);
protected:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_is_connected;
    Address::ptr m_local_address;
    Address::ptr m_remote_address;
};

} // namespace qff


#endif