#ifndef BACKEND_UTIL_H_
#define BACKEND_UTIL_H_

#include <stdlib.h>
#include <string.h>


#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

void msleep(unsigned int msecs);
char * string_strip(char *str);

static inline void * zalloc(size_t size)
{
	return calloc(1, size);
}

#endif /* BACKEND_UTIL_H_ */
