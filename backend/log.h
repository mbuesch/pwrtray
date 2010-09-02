#ifndef LOG_H_
#define LOG_H_

enum {
	LOGLEVEL_ERROR,
	LOGLEVEL_INFO,
	LOGLEVEL_DEBUG,
};

void loginfo(const char *fmt, ...);
void logerr(const char *fmt, ...);
void logdebug(const char *fmt, ...);

#endif /* LOG_H_ */
