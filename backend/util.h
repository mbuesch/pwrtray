#ifndef BACKEND_UTIL_H_
#define BACKEND_UTIL_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "list.h"


#define __stringify(x)		#x
#define stringify(x)		__stringify(x)

#define min(x, y)	({			\
		__typeof__(x) __x = (x);	\
		__typeof__(y) __y = (y);	\
		__x < __y ? __x : __y;		\
	})

#define max(a, b)	({			\
		__typeof__(a) __a = (a);	\
		__typeof__(b) __b = (b);	\
		__a > __b ? __a : __b;		\
	})

#define clamp(v, mi, ma)	max(min(v, ma), mi)

#define round_up(n, s)		((((n) + (s) - 1) / (s)) * (s))

#define div_round_up(x, d)		({	\
		__typeof__(x) __x = (x);	\
		__typeof__(d) __d = (d);	\
		(__x + __d - 1) / __d;		\
	})

#define div_round(x, d)	({			\
		__typeof__(x) __x = (x);	\
		__typeof__(d) __d = (d);	\
		(__x + (__d / 2)) / __d;	\
	})

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

#define compiler_barrier()	__asm__ __volatile__("" : : : "memory")

#define ALIGN(x)		__attribute__((__aligned__(x)))
#define ALIGN_MAX		ALIGN(__BIGGEST_ALIGNMENT__)

void msleep(unsigned int msecs);
char * string_strip(char *str);
char * string_split(char *str, int (*sep_match)(int c));

static inline void * zalloc(size_t size)
{
	return calloc(1, size);
}

static inline int strempty(const char *s)
{
	return (s == NULL) || (s[0] == '\0');
}

uint_fast8_t tiny_hash(const char *str);

pid_t subprocess_exec(const char *_command);

#endif /* BACKEND_UTIL_H_ */
