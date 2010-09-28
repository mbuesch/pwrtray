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

#include "screenlock.h"
#include "log.h"
#include "util.h"

#include "screenlock_n810.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


#define X11LOCK_HELPER		"pwrtray-xlock"
#define X11LOCK_HELPER_PATH	stringify(PREFIX) "/bin/" X11LOCK_HELPER


int block_x11_input(struct screenlock *s)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		logerr("block_x11_input: Failed to fork (%s)\n",
		       strerror(errno));
		return -errno;
	}
	if (pid == 0) { /* Child */
		execl(X11LOCK_HELPER_PATH, X11LOCK_HELPER, NULL);
		logerr("block_x11_input: Failed to exec %s: %s\n",
		       X11LOCK_HELPER_PATH, strerror(errno));
		exit(0);
		while (1);
	}
	s->x11lock_helper = pid;
	logdebug("Forked X11 input blocker helper %s (pid=%d)\n",
		 X11LOCK_HELPER_PATH, (int)pid);

	return 0;
}

void unblock_x11_input(struct screenlock *s)
{
	int err, status;
	pid_t res;

	if (s->x11lock_helper == 0)
		return;

	err = kill(s->x11lock_helper, SIGTERM);
	if (err) {
		logerr("unlock_x11_input: Failed to kill helper process PID %d\n",
		       (int)s->x11lock_helper);
		goto out;
	}
	res = waitpid(s->x11lock_helper, &status, 0);
	if (res < 0) {
		logerr("unlock_x11_input: Failed to waitpid() for PID %d\n",
		       (int)s->x11lock_helper);
		goto out;
	}
	if (status) {
		logerr("unlock_x11_input: The lock helper process "
		       "returned an error code: %d\n", status);
	} else {
		logdebug("Killed X11 input blocker helper (pid=%d)\n",
			 (int)s->x11lock_helper);
	}
out:
	s->x11lock_helper = 0;
}

struct screenlock * screenlock_probe(struct backlight *bl)
{
	struct screenlock *s;

	s = screenlock_n810_probe(bl);
	if (s)
		goto ok;

	return NULL;

ok:
	s->backlight = bl;

	return s;
}

void screenlock_destroy(struct screenlock *s)
{
	if (s) {
		s->destroy(s);
		unblock_x11_input(s);
	}
}
