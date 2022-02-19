#ifndef __QFF_ADDRESS_H__
#define __QFF_ADDRESS_H__

#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <map>

namespace qff {

class Address {
public:
    typedef std::shared_ptr<Address> ptr;

    static Address::ptr Create(const sockaddr* addr, socklen_t addr_len);
    static std::vector<Address::ptr> Lookup(std::string_view host, int family = AF_INET, int type = 0, int protocol = 0);
    static Address::ptr LookupAny(std::string_view host, int family = AF_INET, int type = 0, int protocol = 0);

    static std::vector<std::tuple<std::string, Address::ptr, uint32_t>>
        GetInterfaceAddresses(int family = AF_INET);
    static std::vector<std::pair<Address::ptr, uint32_t>> 
        GetInterfaceAddresses(std::string_view iface, int family = AF_INET);

    virtual ~Address();

    int get_family() const;

    virtual const sockaddr* get_addr() const = 0;
    
    socklen_t get_addr_len() const;
    void set_addr_len(uint32_t v);
    
    std::string to_string();
    virtual std::ostream& dump(std::ostream& os) const = 0;

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator>(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
protected:
    socklen_t m_length;
};

class UnixAddress final : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

    UnixAddress();
    UnixAddress(std::string_view path);

    const sockaddr* get_addr() const override;
    std::string get_path() const;

    std::ostream& dump(std::ostream& os) const override;
private:
    sockaddr_un m_addr;
};

class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    static IPAddress::ptr Create(std::string_view address, uint16_t port = 0);
    static std::shared_ptr<IPAddress> LookupAny(std::string_view host, int family = AF_INET, int type = 0, int protocol = 0);

    virtual IPAddress::ptr broad_cast_address(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr network_address(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnet_address(uint32_t prefix_len) = 0;

    virtual uint32_t get_port() const = 0;
    virtual void set_port(uint16_t v) = 0;
};

class IPv4Address final : public IPAddress {
public:
    typedef std::shared_ptr<IPv4Address> ptr;

    IPv4Address(const sockaddr_in& address) noexcept;
    IPv4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
    const sockaddr* get_addr() const override;
    std::ostream& dump(std::ostream& os) const override;
    IPAddress::ptr broad_cast_address(uint32_t prefix_len) override;
    IPAddress::ptr network_address(uint32_t prefix_len) override;
    IPAddress::ptr subnet_address(uint32_t prefix_len) override;
    uint32_t get_port() const override;
    void set_port(uint16_t v) override;
private:
    sockaddr_in m_addr;
};

class IPv6Address final : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    IPv6Address(const sockaddr_in6& address) noexcept;
    IPv6Address(const uint8_t address[16] = 0, uint16_t port = 0);
    const sockaddr* get_addr() const override;
    std::ostream& dump(std::ostream& os) const override;
    IPAddress::ptr broad_cast_address(uint32_t prefix_len) override;
    IPAddress::ptr network_address(uint32_t prefix_len) override;
    IPAddress::ptr subnet_address(uint32_t prefix_len) override;
    uint32_t get_port() const override;
    void set_port(uint16_t v) override;
private:
    sockaddr_in6 m_addr;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;

    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* get_addr() const override;
    std::ostream& dump(std::ostream& os) const override;
private:
    sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

} //namespace qff

#endif