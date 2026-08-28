#ifndef CUP_TREE_MACROS_H
#define CUP_TREE_MACROS_H

#define __container_of(ptr, type, member)                           \
	({                                                          \
		const typeof(((type *)0)->member) *__mptr = (ptr);  \
		(type *)((char *)__mptr - offset_of(type, member)); \
	})

#define offset_of(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

#endif
