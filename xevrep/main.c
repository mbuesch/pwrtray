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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>


static pid_t report_pid;
static int report_signal;
static unsigned int grace_period_ms;
static struct timeval next_report;


int timeval_after(const struct timeval *a, const struct timeval *b)
{
	if (a->tv_sec > b->tv_sec)
		return 1;
	if ((a->tv_sec == b->tv_sec) && (a->tv_usec > b->tv_usec))
		return 1;
	return 0;
}

void timeval_add_msec(struct timeval *tv, unsigned int msec)
{
	unsigned int seconds, usec;

	seconds = msec / 1000;
	msec = msec % 1000;
	usec = msec * 1000;

	tv->tv_usec += usec;
	while (tv->tv_usec >= 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
	tv->tv_sec += seconds;
}

static void report_event(XIDeviceEvent *ev)
{
	struct timeval now;
	int err;

	gettimeofday(&now, NULL);
	if (!timeval_after(&now, &next_report))
		return;
	next_report = now;
	timeval_add_msec(&next_report, grace_period_ms);

	err = kill(report_pid, report_signal);
	if (err)
		perror("kill");
}

static void signal_handler(int signal)
{
	printf("pwrtray-xevrep: terminated\n");
	exit(0);
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

static void usage(void)
{
	printf("Usage: pwrtray-xevrep PID SIGNAL GRACEPERIOD\n");
}

int main(int argc, char **argv)
{
	Display *display;
	Window window;
	XIEventMask evmask;
	unsigned char evmask_bits[1] = { 0, };
	int res, tmp0, tmp1, xi_opcode;
	int pid;
	sigset_t sigset;

	if (argc != 4) {
		usage();
		return 1;
	}
	if (sscanf(argv[1], "%d", &pid) != 1 ||
	    sscanf(argv[2], "%d", &report_signal) != 1 ||
	    sscanf(argv[3], "%u", &grace_period_ms) != 1) {
		usage();
		return 1;
	}
	report_pid = pid;

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

	res = XQueryExtension(display, "XInputExtension", &xi_opcode, &tmp0, &tmp1);
	if (!res) {
		fprintf(stderr, "X Input extension not available.\n");
		return 1;
	}

	sigemptyset(&sigset);
	res = sigprocmask(SIG_SETMASK, &sigset, NULL);
	res |= install_sighandler(SIGINT, signal_handler);
	res |= install_sighandler(SIGTERM, signal_handler);
	if (res) {
		fprintf(stderr, "Failed to setup signal handlers\n");
		return 1;
	}

	XISetMask(evmask_bits, XI_KeyPress);
	XISetMask(evmask_bits, XI_ButtonPress);
	XISetMask(evmask_bits, XI_Motion);
	memset(&evmask, 0, sizeof(evmask));
	evmask.deviceid = XIAllDevices;
	evmask.mask_len = sizeof(evmask_bits);
	evmask.mask = evmask_bits;

	res = XISelectEvents(display, window, &evmask, 1);
	if (res != Success) {
		fprintf(stderr, "Failed to set event mask (%d)\n", res);
		return 1;
	}

	printf("pwrtray-xevrep: Monitoring X11 input events...\n");
	while (1) {
		XEvent ev;
		XGenericEventCookie *cookie = &ev.xcookie;

		XNextEvent(display, &ev);
		if (XGetEventData(display, cookie) &&
		    cookie->type == GenericEvent &&
		    cookie->extension == xi_opcode) {
			XIDeviceEvent *event = cookie->data;

			report_event(event);
		}
		XFreeEventData(display, cookie);
	}
}
