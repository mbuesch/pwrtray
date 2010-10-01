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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>


static Display *display;


static void signal_handler(int signal)
{
	XUngrabKeyboard(display, CurrentTime);
	XUngrabPointer(display, CurrentTime);
	printf("pwrtray-xlock: released\n");
	_Exit(0);
}

static int install_sighandler(int signal, void (*handler)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = handler;

	return sigaction(signal, &act, NULL);
}

int main(int argc, char **argv)
{
	Window window;
	int res;
	sigset_t sigset;

	display = XOpenDisplay(NULL);
	if (!display) {
		/* Fallback to the first display. */
		display = XOpenDisplay(":0");
		if (!display) {
			fprintf(stderr, "Failed to open DISPLAY\n");
			return 1;
		}
	}
	window = DefaultRootWindow(display);

	sigemptyset(&sigset);
	res = sigprocmask(SIG_SETMASK, &sigset, NULL);
	res |= install_sighandler(SIGINT, signal_handler);
	res |= install_sighandler(SIGTERM, signal_handler);
	if (res) {
		fprintf(stderr, "Failed to setup signal handlers\n");
		return 1;
	}

	res = XGrabPointer(display, window,
			   False, 0, GrabModeAsync, GrabModeAsync,
			   None, None, CurrentTime);
	if (res != GrabSuccess) {
		fprintf(stderr, "Failed to grab pointer (res=%d)\n", res);
		return 1;
	}
	res = XGrabKeyboard(display, window,
			    False, GrabModeAsync, GrabModeAsync,
			    CurrentTime);
	if (res != GrabSuccess) {
		fprintf(stderr, "Failed to grab keyboard (res=%d)\n", res);
		XUngrabPointer(display, CurrentTime);
		return 1;
	}

	printf("pwrtray-xlock: locked\n");
	while (1)
		sleep(0xFFFF);
}
