/*
 *   Copyright (C) 2011 Michael Buesch <m@bues.ch>
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

#include "xevrep.h"
#include "log.h"
#include "util.h"
#include "conf.h"
#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>


#define XEVREP_HELPER		"pwrtray-xevrep"
#define XEVREP_HELPER_PATH	stringify(PREFIX) "/bin/" XEVREP_HELPER


int xevrep_enable(struct xevrep *xr)
{
	pid_t pid;
	int grace_period;
	char arg1[32], arg2[32], arg3[32];

#ifndef FEATURE_XEVREP
# warning "X11 input event reporting feature disabled"
	logdebug("X11 input event reporting feature disabled");
	return 0;
#endif

	if (!xr)
		return 0;
	if (xr->helper_pid)
		return 0;
	grace_period = config_get_int(backend.config,
				      "XEVREP", "grace_period",
				      1000);
	snprintf(arg1, sizeof(arg1), "%d", getpid());
	snprintf(arg2, sizeof(arg2), "%d", SIGUSR1);
	snprintf(arg3, sizeof(arg3), "%d", grace_period);
	pid = vfork();
	if (pid < 0) {
		logerr("xevrep_enablet: Failed to fork (%s)\n",
		       strerror(errno));
		return -errno;
	}
	if (pid == 0) { /* Child */
		execl(XEVREP_HELPER_PATH, XEVREP_HELPER,
		      arg1, arg2, arg3, NULL);
		logerr("xevrep_enable: Failed to exec %s: %s\n",
		       XEVREP_HELPER_PATH, strerror(errno));
		exit(0);
		while (1);
	}
	xr->helper_pid = pid;
	logdebug("Forked X11 input event helper %s (pid=%d)\n",
		 XEVREP_HELPER_PATH, (int)pid);

	return 0;
}

void xevrep_disable(struct xevrep *xr)
{
	int err;
	pid_t pid;

#ifndef FEATURE_XEVREP
	return;
#endif

	if (!xr)
		return;
	pid = xr->helper_pid;
	if (!pid)
		return;

	block_signals();

	xr->killed = 1;
	err = kill(pid, SIGTERM);
	if (err) {
		logerr("xevrep_disable: Failed to kill helper process PID %d\n",
		       (int)pid);
	}

	unblock_signals();
}

void xevrep_sigchld(struct xevrep *xr, int wait)
{
	int status;
	pid_t pid, res;

#ifndef FEATURE_XEVREP
	return;
#endif

	if (!xr)
		return;
	pid = xr->helper_pid;
	if (!pid)
		return;

	res = waitpid(pid, &status, wait ? 0 : WNOHANG);
	if (res < 0) {
		if (errno == ECHILD)
			return;
		logerr("xevrep_sigchld: waitpid failed for PID %d: %s\n",
		       (int)pid, strerror(errno));
		return;
	}
	if (!xr->killed) {
		logerr("xevrep_sigchld: X11 input event helper was "
		       "murdered by a stranger.\n");
	}
	xr->helper_pid = 0;
	xr->killed = 0;

	if (status) {
		logerr("xevrep_sigchld: The helper process "
		       "returned an error code: %d\n", status);
	} else {
		logdebug("X11 input event helper (pid=%d) terminated\n",
			 (int)pid);
	}
}
