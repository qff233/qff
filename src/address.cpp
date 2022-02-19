#include "address.h"

#include "log.h"

#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

namespace qff {

template<typename T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

template<class T>
static uint32_t CountBytes(T value) {
    uint32_t result = 0;
    for(; value; ++result) {
        value &= value - 1;
    }
    return result;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addr_len) {
    if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family) {
        case AF_INET:
            result.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

std::vector<Address::ptr> Address::Lookup(std::string_view host, int family, int type, int protocol) {
    addrinfo hints, *results, *next;

    hints.ai_flags = 0;
    hints.ai_family = family;
    hints.ai_socktype = type;
    hints.ai_protocol = protocol;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::vector<Address::ptr> vec_result;
    std::string_view node;
    const char* service = NULL;

    //check ipv6address format
    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.data() + 1, ']', host.size() - 1);
        if(endipv6) {
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.data() - 1);
        }
    }

    //check node format
    if(node.empty()) {
        service = (const char*)memchr(host.data(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.data() + host.size() - service - 1)) {
                node = host.substr(0, service - host.data());
                ++service;
            }
        }
    }

    if(node.empty()) {
        node = host;
    }

    int error = getaddrinfo(node.data(), service, &hints, &results);
    if(error) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error);
        return std::vector<Address::ptr>();
    }

    next = results;
    while(next) {
        vec_result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        //SYLAR_LOG_INFO(g_logger) << ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return vec_result;
}

Address::ptr Address::LookupAny(std::string_view host, int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    result = std::move(Lookup(host, family, type, protocol));
    if(result.empty())
        return nullptr;
    return result[0];
}

std::vector<std::tuple<std::string, Address::ptr, uint32_t>>
        Address::GetInterfaceAddresses(int family) {
    std::vector<std::tuple<std::string, Address::ptr, uint32_t>> result;
    struct ifaddrs *next, *results;
    if(getifaddrs(&results) != 0) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Address::GetInterfaceAddresses getifaddrs "
            " err=" << errno << " errstr=" << strerror(errno);
        return std::vector<std::tuple<std::string, Address::ptr, uint32_t>>();
    }

    try {
        for(next = results; next; next = next->ifa_next) {
            Address::ptr addr;
            uint32_t prefix_len = ~0u;
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) {
                continue;
            }
            switch(next->ifa_addr->sa_family) {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) {
                result.push_back({next->ifa_name, addr, prefix_len});
            }
        }
    } catch (...) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "Address::GetInterfaceAddresses exception";
        freeifaddrs(results);
        return std::vector<std::tuple<std::string, Address::ptr, uint32_t>>();
    }
    freeifaddrs(results);
    return result;
}

std::vector<std::pair<Address::ptr, uint32_t>> 
        Address::GetInterfaceAddresses(std::string_view iface, int family) {
    std::vector<std::pair<Address::ptr, uint32_t>> result;
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return std::vector<std::pair<Address::ptr, uint32_t>>();
    }

    std::vector<std::tuple<std::string, Address::ptr, uint32_t>> results;

    results = GetInterfaceAddresses(family);
    if(results.empty())
        return std::vector<std::pair<Address::ptr, uint32_t>>();

    for(auto &i : results) {
        auto[name, address, prefix_len] = i;
        if(name == iface)
            result.push_back({address, prefix_len});
    }
    return result;
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
    dump(ss);
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

std::ostream& UnixAddress::dump(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path)
            && m_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

IPAddress::ptr IPAddress::Create(std::string_view address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address.data(), NULL, &hints, &results);
    if(error) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->set_port(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

IPAddress::ptr IPAddress::LookupAny(std::string_view host, int family, int type, int protocol) {
    Address::ptr result;
    result = std::move(Address::LookupAny(host, family, type, protocol));
    if(result)
        return std::dynamic_pointer_cast<IPAddress>(result);
    return nullptr;
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
    return (sockaddr*)&m_addr;
}

std::ostream& IPv4Address::dump(std::ostream& os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broad_cast_address(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr 
        |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(baddr);
}

IPAddress::ptr IPv4Address::network_address(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr 
        &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(baddr);
}

IPAddress::ptr IPv4Address::subnet_address(uint32_t prefix_len) {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::get_port() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::set_port(uint16_t v) {
    m_addr.sin_port = byteswapOnLittleEndian(v);
}

IPv6Address::IPv6Address(const sockaddr_in6& address) noexcept
    :m_addr(address) {
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&m_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::get_addr() const {
    return (sockaddr*)&m_addr;
}

std::ostream& IPv6Address::dump(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broad_cast_address(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::network_address(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::subnet_address(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::get_port() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::set_port(uint16_t v) {
    m_addr.sin6_port = byteswapOnLittleEndian(v);
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

std::ostream& UnknownAddress::dump(std::ostream& os) const {
    os << "[UnknownAddress family=" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.dump(os);
}

} //namespace qff