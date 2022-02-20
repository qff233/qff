#ifndef __QFF_BYTE_ARRAY_H__
#define __QFF_BYTE_ARRAY_H__

#include <memory>
#include <vector>
#include <sys/uio.h>

namespace qff {
    
class ByteArray {
public:
    typedef std::shared_ptr<ByteArray> ptr;

    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    void write_F_int8  (int8_t value);
    void write_F_uint8 (uint8_t value);
    void write_F_int16 (int16_t value);
    void write_F_uint16(uint16_t value);
    void write_F_int32 (int32_t value);
    void write_F_uint32(uint32_t value);
    void write_F_int64 (int64_t value);
    void write_F_uint64(uint64_t value);
    void write_int32  (int32_t value);
    void write_uint32 (uint32_t value);
    void write_int64  (int64_t value);
    void write_uint64 (uint64_t value);
    void write_float  (float value);
    void write_double (double value);
    void write_string_F16(std::string_view value);
    void write_string_F32(std::string_view value);
    void write_string_F64(std::string_view value);
    void write_string_Vint(std::string_view value);
    void write_string_without_length(std::string_view value);

    int8_t   read_F_int8();
    uint8_t  read_F_uint8();
    int16_t  read_F_int16();
    uint16_t read_F_uint16();
    int32_t  read_F_int32();
    uint32_t read_F_uint32();
    int64_t  read_F_int64();
    uint64_t read_F_uint64();
    int32_t  read_int32();
    uint32_t read_uint32();
    int64_t  read_int64();
    uint64_t read_uint64();
    float    read_float();
    double   read_double();

    std::string read_string_F16();
    std::string read_string_F32();
    std::string read_string_F64();
    std::string read_string_Vint();

    void clear() noexcept;

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    size_t get_position() const noexcept { return m_position;}
    void set_position(size_t v);

    bool write_to_file(const std::string& name) const;
    bool read_from_file(const std::string& name);

    size_t get_base_size() const noexcept { return m_baseSize;}
    size_t get_read_size() const noexcept { return m_size - m_position;}

    bool is_little_endian() const noexcept;
    void set_is_little_endian(bool val) noexcept;

    std::string to_string() const;
    std::string to_hex_string() const;

    uint64_t get_read_buffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const noexcept;
    uint64_t get_read_buffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const noexcept;
    uint64_t get_write_buffers(std::vector<iovec>& buffers, uint64_t len) noexcept;
    size_t get_size() const noexcept { return m_size;}
private:
    void add_capacity(size_t size);
    size_t get_capacity() const noexcept;
private:
    size_t m_baseSize;
    size_t m_position;
    size_t m_capacity;
    size_t m_size;
    int8_t m_endian;
    Node* m_root;
    Node* m_cur;
};

} // namespace qff


#endif