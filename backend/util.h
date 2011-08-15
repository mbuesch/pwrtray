#ifndef BACKEND_UTIL_H_
#define BACKEND_UTIL_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "list.h"


#define __stringify(x)		#x
#define stringify(x)		__stringify(x)

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))
#define clamp(v, mi, ma)	max(min(v, ma), mi)
#define round_up(n, s)		((((n) + (s) - 1) / (s)) * (s))

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

#define compiler_barrier()	__asm__ __volatile__("" : : : "memory")

void msleep(unsigned int msecs);
char * string_strip(char *str);

static inline void * zalloc(size_t size)
{
	return calloc(1, size);
}

uint_fast8_t tiny_hash(const char *str);

#endif /* BACKEND_UTIL_H_ */
