#include "byte_array.h"

#include <string.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include "macro.h"
#include "log.h"

namespace qff {
    
ByteArray::Node::Node(size_t s)
    :ptr(new char[s])
    ,next(nullptr)
    ,size(s) {
}

ByteArray::Node::Node()
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0) {
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
        ptr = nullptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(QFF_BIG_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

void ByteArray::write_F_int8  (int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::write_F_uint8 (uint8_t value) {
    write(&value, sizeof(value));
}
void ByteArray::write_F_int16 (int16_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::write_F_uint16(uint16_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::write_F_int32 (int32_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::write_F_uint32(uint32_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::write_F_int64 (int64_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::write_F_uint64(uint64_t value) {
    if(m_endian != QFF_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}


void ByteArray::write_int32  (int32_t value) {
    write_uint32(EncodeZigzag32(value));
}

void ByteArray::write_uint32 (uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::write_int64  (int64_t value) {
    write_uint64(EncodeZigzag64(value));
}

void ByteArray::write_uint64 (uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::write_float  (float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    write_F_uint32(v);
}

void ByteArray::write_double (double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    write_F_uint64(v);
}

void ByteArray::write_string_F16(std::string_view value) {
    write_F_uint16(value.size());
    write(value.data(), value.size());
}

void ByteArray::write_string_F32(std::string_view value) {
    write_F_uint32(value.size());
    write(value.data(), value.size());
}

void ByteArray::write_string_F64(std::string_view value) {
    write_F_uint64(value.size());
    write(value.data(), value.size());
}

void ByteArray::write_string_Vint(std::string_view value) {
    write_uint64(value.size());
    write(value.data(), value.size());
}

void ByteArray::write_string_without_length(std::string_view value) {
    write(value.data(), value.size());
}

int8_t   ByteArray::read_F_int8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t  ByteArray::read_F_uint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(TYPE) \
    TYPE v; \
    read(&v, sizeof(v)); \
    if(m_endian == QFF_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t  ByteArray::read_F_int16() {
    XX(int16_t);
}
uint16_t ByteArray::read_F_uint16() {
    XX(uint16_t);
}

int32_t  ByteArray::read_F_int32() {
    XX(int32_t);
}

uint32_t ByteArray::read_F_uint32() {
    XX(uint32_t);
}

int64_t  ByteArray::read_F_int64() {
    XX(int64_t);
}

uint64_t ByteArray::read_F_uint64() {
    XX(uint64_t);
}

#undef XX

int32_t  ByteArray::read_int32() {
    return DecodeZigzag32(read_uint32());
}

uint32_t ByteArray::read_uint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = read_F_uint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

int64_t  ByteArray::read_int64() {
    return DecodeZigzag64(read_uint64());
}

uint64_t ByteArray::read_uint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = read_F_uint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

float    ByteArray::read_float() {
    uint32_t v = read_F_uint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double   ByteArray::read_double() {
    uint64_t v = read_F_uint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::read_string_F16() {
    uint16_t len = read_F_uint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::read_string_F32() {
    uint32_t len = read_F_uint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::read_string_F64() {
    uint64_t len = read_F_uint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::read_string_Vint() {
    uint64_t len = read_uint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear() noexcept {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}

void ByteArray::write(const void* buf, size_t size) {
    if(size == 0) {
        return;
    }
    add_capacity(size);

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;

    while(size > 0) {
        if(ncap >= size) {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }

    if(m_position > m_size) {
        m_size = m_position;
    }
}

void ByteArray::read(void* buf, size_t size) {
if(size > get_read_size()) {
        throw std::out_of_range("no enough length");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

void ByteArray::set_position(size_t v) {
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

bool ByteArray::write_to_file(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = get_read_size();
    int64_t pos = m_position;
    Node* cur = m_cur;

    while(read_size > 0) {
        int diff = pos % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}

bool ByteArray::read_from_file(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        QFF_LOG_ERROR(QFF_LOG_SYSTEM) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

bool ByteArray::is_little_endian() const noexcept {
    return m_endian == QFF_LITTLE_ENDIAN;
}

void ByteArray::set_is_little_endian(bool val) noexcept {
    if(val)
        m_endian = QFF_LITTLE_ENDIAN;
    else
        m_endian = QFF_BIG_ENDIAN;
}

std::string ByteArray::to_string() const {
    std::string str;
    str.resize(get_read_size());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

std::string ByteArray::to_hex_string() const {
    std::string str = to_string();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

uint64_t ByteArray::get_read_buffers(std::vector<iovec>& buffers, uint64_t len) const noexcept {
    len = len > get_read_size() ? get_read_size() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::get_read_buffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const noexcept {
    len = len > get_read_size() ? get_read_size() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::get_write_buffers(std::vector<iovec>& buffers, uint64_t len) noexcept {
    if(len == 0) {
        return 0;
    }
    add_capacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}


void ByteArray::add_capacity(size_t size) {
    if(size == 0) {
        return;
    }
    size_t old_cap = get_capacity();
    if(old_cap >= size) {
        return;
    }

    size = size - old_cap;
    size_t count = ceil(1.0 * size / m_baseSize);
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    if(old_cap == 0) {
        m_cur = first;
    }
}

size_t ByteArray::get_capacity() const noexcept{ 
    return m_capacity - m_position;
}


} // namespace qff