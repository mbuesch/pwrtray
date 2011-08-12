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

#include "x11lock.h"
#include "log.h"
#include "util.h"
#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


#define X11LOCK_HELPER		"pwrtray-xlock"
#define X11LOCK_HELPER_PATH	stringify(PREFIX) "/bin/" X11LOCK_HELPER


int block_x11_input(struct x11lock *xl)
{
	pid_t pid;

#ifndef FEATURE_XLOCK
# warning "X11 input blocking feature disabled"
	logdebug("X11 input blocking feature disabled\n");
	return 0;
#endif

	if (!xl)
		return 0;
	if (xl->helper_pid)
		return 0;
	pid = vfork();
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
	xl->helper_pid = pid;
	logdebug("Forked X11 input blocker helper %s (pid=%d)\n",
		 X11LOCK_HELPER_PATH, (int)pid);

	return 0;
}

void unblock_x11_input(struct x11lock *xl)
{
	int err;
	pid_t pid;

#ifndef FEATURE_XLOCK
	return;
#endif

	if (!xl)
		return;
	pid = xl->helper_pid;
	if (!pid)
		return;

	block_signals();

	xl->killed = 1;
	err = kill(pid, SIGTERM);
	if (err) {
		logerr("unlock_x11_input: Failed to kill helper process PID %d\n",
		       (int)pid);
		return;
	}

	unblock_signals();
}

void x11lock_sigchld(struct x11lock *xl, int wait)
{
	int status;
	pid_t pid, res;

#ifndef FEATURE_XLOCK
	return;
#endif

	if (!xl)
		return;
	pid = xl->helper_pid;
	if (!pid)
		return;

	res = waitpid(pid, &status, wait ? 0 : WNOHANG);
	if (res < 0) {
		if (errno == ECHILD)
			return;
		logerr("x11lock_sigchld: waitpid failed for PID %d: %s\n",
		       (int)pid, strerror(errno));
		return;
	}
	if (!xl->killed) {
		logerr("x11lock_sigchld: X11 lock helper was "
		       "murdered by a stranger.\n");
	}
	xl->helper_pid = 0;
	xl->killed = 0;

	if (status) {
		logerr("x11lock_sigchld: The helper process "
		       "returned an error code: %d\n", status);
	} else {
		logdebug("X11 lock helper (pid=%d) terminated\n",
			 (int)pid);
	}
}
