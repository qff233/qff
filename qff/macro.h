#ifndef __QFF_MACRO_H__
#define __QFF_MACRO_H__

#include <byteswap.h>
#include <stdint.h>

#define NONECOPYABLE(CLASS_NAME)								\
	CLASS_NAME(const CLASS_NAME&)  = delete;					\
	CLASS_NAME(CLASS_NAME&&)       = delete;					\
	CLASS_NAME& operator=(const CLASS_NAME&&) = delete;			\
	CLASS_NAME& operator=(CLASS_NAME&&)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define QFF_LITTLE_ENDIAN 1
#define QFF_BIG_ENDIAN 2

#if BYTE_ORDER == BIG_ENDIAN
#define QFF_BYTE_ORDER QFF_BIG_ENDIAN
#else
#define QFF_BYTE_ORDER QFF_LITTLE_ENDIAN
#endif

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint8_t), T>::type
byteswap(T value) {
    return value;
}

#if QFF_BYTE_ORDER == QFF_BIG_ENDIAN
template<class T>
T ByteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T ByteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else
template<class T>
T ByteswapOnLittleEndian(T t) {
    return byteswap(t);
}
template<class T>
T ByteswapOnBigEndian(T t) {
    return t;
}
#endif


#endif
