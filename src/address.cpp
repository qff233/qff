#include "address.h"

#include "log.h"

#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

namespace qff {

Address::ptr Address::Create(const sockaddr* addr, socklen_t addr_len) {

}

std::vector<Address::ptr> Address::Lookup(std::string_view host, int family = AF_INET, int type, int protocol) {

}

Address::ptr Address::LookupAny(const std::string& host, int family = AF_INET, int type, int protocol) {

}

std::vector<std::tuple<std::string, Address::ptr, uint32_t>>
        Address::GetInterfaceAddresses(int family = AF_INET) {

}

std::vector<std::pair<Address::ptr, uint32_t>> 
        Address::GetInterfaceAddresses(std::string_view iface, int family = AF_INET) {

}
 
Address::~Address() {
}

int Address::get_family() const {
    return get_addr()->sa_family;
}

socklen_t Address::get_addr_len() const {
    return m_length;
}

void Address::set_addr_len(uint32_t v) {
    m_length = v;
}

std::string Address::to_string() {
    std::stringstream ss;
    out_put(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const {
    socklen_t minlen = std::min(m_length, rhs.m_length);
    int result = ::memcmp(get_addr(), rhs.get_addr(), minlen);
    if(result < 0) {
        return true;
    } else if(result > 0) {
        return false;
    }
    if(m_length < rhs.m_length)
        return true;
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return m_length == rhs.m_length
        && memcmp(this->get_addr(), rhs.get_addr(), m_length) == 0;
}

bool Address::operator>(const Address& rhs) const {
    if(*this < rhs || *this == rhs)
        return false;
    return true;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this==rhs);
}

static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(std::string_view path) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --m_length;
    }

    if(m_length > sizeof(m_addr.sun_path)) {
        throw std::logic_error("path too long");
    }

    memcpy(m_addr.sun_path, path.data(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::get_addr() const {
    return (sockaddr*)&m_addr;
}

std::string UnixAddress::get_path() const {
    std::stringstream ss;
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << m_addr.sun_path;
    }
    return ss.str();
}

std::ostream& UnixAddress::out_put(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

IPAddress::ptr IPAddress::Create(std::string_view address, uint16_t port) {

}

std::shared_ptr<IPAddress> IPAddress::LookupAny(const std::string& host, int family, int type, int protocol) {

}

IPv4Address::IPv4Address(const sockaddr_in& address) noexcept
    :m_addr(address) {
}

IPv4Address::IPv4Address(uint32_t address, uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = byteswapOnLittleEndian(port);
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::get_addr() const {

}

std::ostream& IPv4Address::out_put(std::ostream& os) const {

}

IPAddress::ptr IPv4Address::broad_cast_address(uint32_t prefix_len) {

}

IPAddress::ptr IPv4Address::network_address(uint32_t prefix_len) {

}

IPAddress::ptr IPv4Address::subnet_address(uint32_t prefix_len) {

}

uint32_t IPv4Address::get_port() const {

}

void IPv4Address::set_port(uint16_t v) {

}

IPv6Address::IPv6Address(const sockaddr_in6& address) 
    :m_addr(address) {
}

IPv6Address::IPv6Address(uint32_t address = INADDR_ANY, uint32_t port) {

}

const sockaddr* IPv6Address::get_addr() const {

}

std::ostream& IPv6Address::out_put(std::ostream& os) const {

}

IPAddress::ptr IPv6Address::broad_cast_address(uint32_t prefix_len) {

}

IPAddress::ptr IPv6Address::network_address(uint32_t prefix_len) {

}

IPAddress::ptr IPv6Address::subnet_address(uint32_t prefix_len) {

}

uint32_t IPv6Address::get_port() const {

}

void IPv6Address::set_port(uint16_t v) {

}

UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) 
    :m_addr(addr) {
}

const sockaddr* UnknownAddress::get_addr() const {
    return (sockaddr*)&m_addr;
}

std::ostream& UnknownAddress::out_put(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.out_put(os);
}

} //namespace qff