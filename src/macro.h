#ifndef __QFF_MACRO_H__
#define __QFF_MACRO_H__

#define NONECOPYABLE(CLASS_NAME)								\
	CLASS_NAME(const CLASS_NAME&)  = delete;					\
	CLASS_NAME(CLASS_NAME&&)       = delete;					\
	CLASS_NAME& operator=(const CLASS_NAME&&) = delete;			\
	CLASS_NAME& operator=(CLASS_NAME&&)

#endif
