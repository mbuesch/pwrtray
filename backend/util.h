#ifndef BACKEND_UTIL_H_
#define BACKEND_UTIL_H_

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

void msleep(unsigned int msecs);
char * string_strip(char *str);

#endif /* BACKEND_UTIL_H_ */
