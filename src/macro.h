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

#if BYTE_ORDER == BIG_ENDIAN
#define QFF_BYTE_ORDER QFF_BIG_ENDIAN
#else
#define QFF_BYTE_ORDER QFF_LITTLE_ENDIAN
#endif

#if QFF_BYTE_ORDER == QFF_BIG_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

#endif
