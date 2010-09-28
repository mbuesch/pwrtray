/*
 *   Copyright (C) 2010 Michael Buesch <mb@bu3sch.de>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "log.h"
#include "args.h"

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>


static FILE *log_fd;


static void write_log(FILE *fd, int prio, const char *fmt, va_list args)
{
	if (log_fd) {
		vfprintf(log_fd, fmt, args);
		fflush(log_fd);
	} else {
		if (cmdargs.background)
			vsyslog(prio, fmt, args);
		else
			vfprintf(fd, fmt, args);
	}
}

void log_initialize(void)
{
	if (cmdargs.logfile) {
		log_fd = fopen(cmdargs.logfile, "w+");
		if (!log_fd)
			logerr("Failed to open logfile: %s\n", cmdargs.logfile);
	}
}

void log_exit(void)
{
	if (log_fd)
		fclose(log_fd);
	log_fd = NULL;
}

void loginfo(const char *fmt, ...)
{
	va_list args;

	if (loglevel_is_info()) {
		va_start(args, fmt);
		write_log(stdout, LOG_MAKEPRI(LOG_DAEMON, LOG_INFO), fmt, args);
		va_end(args);
	}
}

void logerr(const char *fmt, ...)
{
	va_list args;

	if (loglevel_is_error()) {
		va_start(args, fmt);
		write_log(stderr, LOG_MAKEPRI(LOG_DAEMON, LOG_ERR), fmt, args);
		va_end(args);
	}
}

void _logdebug(const char *fmt, ...)
{
	va_list args;

	if (loglevel_is_debug()) {
		va_start(args, fmt);
		write_log(stdout, LOG_MAKEPRI(LOG_DAEMON, LOG_DEBUG), fmt, args);
		va_end(args);
	}
}
