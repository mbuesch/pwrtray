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


void loginfo(const char *fmt, ...)
{
	va_list args;

	if (cmdargs.loglevel < LOGLEVEL_INFO)
		return;
	va_start(args, fmt);
	if (cmdargs.background)
		vsyslog(LOG_MAKEPRI(LOG_DAEMON, LOG_INFO), fmt, args);
	else
		vfprintf(stdout, fmt, args);
	va_end(args);
}

void logerr(const char *fmt, ...)
{
	va_list args;

	if (cmdargs.loglevel < LOGLEVEL_ERROR)
		return;
	va_start(args, fmt);
	if (cmdargs.background)
		vsyslog(LOG_MAKEPRI(LOG_DAEMON, LOG_ERR), fmt, args);
	else
		vfprintf(stderr, fmt, args);
	va_end(args);
}

void logdebug(const char *fmt, ...)
{
	va_list args;

	if (cmdargs.loglevel < LOGLEVEL_DEBUG)
		return;
	va_start(args, fmt);
	if (cmdargs.background) {
		vsyslog(LOG_MAKEPRI(LOG_DAEMON, LOG_DEBUG), fmt, args);
	} else {
		fprintf(stdout, "[debug]: ");
		vfprintf(stdout, fmt, args);
	}
	va_end(args);
}
