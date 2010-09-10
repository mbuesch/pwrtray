#ifndef LOG_H_
#define LOG_H_

#include "args.h"

enum {
	LOGLEVEL_ERROR,
	LOGLEVEL_INFO,
	LOGLEVEL_DEBUG,
	LOGLEVEL_VERBOSE,
};

#define loglevel_is_error()	(cmdargs.loglevel >= LOGLEVEL_ERROR)
#define loglevel_is_info()	(cmdargs.loglevel >= LOGLEVEL_INFO)
#define loglevel_is_debug()	(cmdargs.loglevel >= LOGLEVEL_DEBUG)
#define loglevel_is_verbose()	(cmdargs.loglevel >= LOGLEVEL_VERBOSE)

void loginfo(const char *fmt, ...);
void logerr(const char *fmt, ...);
void logdebug(const char *fmt, ...);
#define logverbose(...) do {			\
		if (loglevel_is_verbose())	\
			logdebug(__VA_ARGS__);	\
	} while (0)

void log_initialize(void);
void log_exit(void);

#endif /* LOG_H_ */
