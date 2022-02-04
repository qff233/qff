#ifndef __QFF_MACRO_H__
#define __QFF_MACRO_H__

#define NONECOPYABLE(CLASS_NAME)								\
	CLASS_NAME(const CLASS_NAME&)  = delete;					\
	CLASS_NAME(CLASS_NAME&&)       = delete;					\
	CLASS_NAME& operator=(const CLASS_NAME&&) = delete;			\
	CLASS_NAME& operator=(CLASS_NAME&&)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif
