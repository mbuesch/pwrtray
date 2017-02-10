/*
 *   Copyright (C) 2010 Michael Buesch <m@bues.ch>
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

#include "util.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>


void msleep(unsigned int msecs)
{
	int err;
	struct timespec time;

	time.tv_sec = 0;
	while (msecs >= 1000) {
		time.tv_sec++;
		msecs -= 1000;
	}
	time.tv_nsec = msecs;
	time.tv_nsec *= 1000000;
	do {
		err = nanosleep(&time, &time);
	} while (err && errno == EINTR);
	if (err) {
		fprintf(stderr, "nanosleep() failed with: %s\n",
			strerror(errno));
	}
}

char * string_strip(char *str)
{
	char *start = str;
	size_t len;

	if (!str)
		return NULL;
	while (*start != '\0' && isspace(*start))
		start++;
	len = strlen(start);
	while (len && isspace(start[len - 1])) {
		start[len - 1] = '\0';
		len--;
	}

	return start;
}

char * string_split(char *str, int (*sep_match)(int c))
{
	char c;

	if (!str)
		return NULL;
	for (c = *str; c != '\0'; c = *str) {
		if (sep_match(c)) {
			*str = '\0';
			return str + 1;
		}
		str++;
	}

	return NULL;
}

uint_fast8_t tiny_hash(const char *str)
{
	uint8_t c, hash = 21;

	while ((c = *str++) != '\0')
		hash = ((hash << 3) + hash) + c;

	return hash;
}

pid_t subprocess_exec(const char *_command)
{
	char command_buf[4096] = { };
	char *command;
	char *argv[32] = { };
	unsigned int i;
	long err = 0;
	pid_t pid;

	if (strlen(_command) >= sizeof(command_buf) - 1) {
		logerr("subprocess_exec: Command too long");
		err = -E2BIG;
		goto out;
	}
	strcpy(command_buf, _command);
	command = command_buf;

	for (i = 0; i < ARRAY_SIZE(argv) - 1; i++) {
		argv[i] = command;
		command = string_split(command, isspace);
		if (strempty(argv[i])) {
			/* Skip empty elements */
			argv[i] = NULL;
			i--;
		}
		if (!command)
			break;
	}
	if (command) {
		logerr("subprocess_exec: Too many arguments: '%s'\n", _command);
		err = -EINVAL;
		goto out;
	}
	if (!argv[0]) {
		logerr("subprocess_exec: Too few arguments\n");
		err = -EINVAL;
		goto out;
	}

	pid = vfork();
	if (pid < 0) {
		logerr("subprocess_exec: Failed to fork (%s)\n", strerror(errno));
		err = -errno;
		goto out;
	}
	if (pid == 0) { /* Child */
		execv(argv[0], argv);
		logerr("subprocess_exec: Failed to exec '%s': %s\n",
		       _command, strerror(errno));
		exit(0);
		while (1);
	}
	err = pid;
out:
	return err;
}
