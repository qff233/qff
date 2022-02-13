#include "address.h"

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

}

socklen_t Address::get_addr_len() const {

}

void Address::set_addr_len(uint32_t v) {

}

std::string Address::to_string() {

}

std::ostream& Address::out_put(std::ostream& os) const {

}

bool Address::operator<(const Address& rhs) const {

}

bool Address::operator==(const Address& rhs) const {

}

bool Address::operator>(const Address& rhs) const {

}

bool Address::operator!=(const Address& rhs) const {

}

UnixAddress::UnixAddress() {

}

UnixAddress::UnixAddress(std::string_view path) {

}

const sockaddr* UnixAddress::get_addr() const {

}

std::string UnixAddress::get_path() const {

}

std::ostream& UnixAddress::out_put(std::ostream& os) const {

}

IPAddress::ptr IPAddress::Create(std::string_view address, uint16_t port) {

}

std::shared_ptr<IPAddress> IPAddress::LookupAny(const std::string& host, int family, int type, int protocol) {

}

IPv4Address::IPv4Address(uint32_t address = INADDR_ANY, uint32_t port) {

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

}

UnknownAddress::UnknownAddress(const sockaddr& addr) {

}

const sockaddr* UnknownAddress::get_addr() const {

}

std::ostream& UnknownAddress::out_put(std::ostream& os) const {

}

} //namespace qff