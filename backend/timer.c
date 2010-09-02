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

#include "timer.h"
#include "main.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


static LIST_HEAD(timer_list);


static void timespec_add_msec(struct timespec *ts, unsigned int msec)
{
	unsigned int seconds, nsec;

	seconds = msec / 1000;
	msec = msec % 1000;
	nsec = msec * 1000000;

	ts->tv_nsec += nsec;
	while (ts->tv_nsec >= 1000000000) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000;
	}
	ts->tv_sec += seconds;
}

static int timespec_bigger(const struct timespec *a, const struct timespec *b)
{
	if (a->tv_sec > b->tv_sec)
		return 1;
	if ((a->tv_sec == b->tv_sec) && (a->tv_nsec > b->tv_nsec))
		return 1;
	return 0;
}

void sleeptimer_init(struct sleeptimer *timer,
		     sleeptimer_callback_t callback)
{
	memset(timer, 0, sizeof(*timer));
	timer->callback = callback;
}

void sleeptimer_set_timeout_relative(struct sleeptimer *timer,
				     unsigned int msecs)
{
	timer->timeout.tv_sec = 0;
	timer->timeout.tv_nsec = 0;
	timespec_add_msec(&timer->timeout, msecs);
}

int sleeptimer_enqueue(struct sleeptimer *timer)
{
	struct sleeptimer *i;
	int inserted = 0;

	INIT_LIST_HEAD(&timer->list);

//FIXME
	list_for_each_entry(i, &timer_list, list) {
		if (timespec_bigger(&i->timeout, &timer->timeout)) {
			list_add_tail(&timer->list, &i->list);
			inserted = 1;
			break;
		}
	}
	if (!inserted)
		list_add_tail(&timer->list, &timer_list);

	return 0;
}

void sleeptimer_dequeue(struct sleeptimer *timer)
{
	list_del(&timer->list);
}

int sleeptimer_wait_next(void)
{
	struct sleeptimer *next_timer;
	int err;

	if (list_empty(&timer_list))
		return -ENOENT;
	next_timer = list_first_entry(&timer_list, struct sleeptimer, list);

	while (1) {
		err = nanosleep(&next_timer->timeout, &next_timer->timeout);
		if (!err)
			break;
		if (err && errno != EINTR) {
			logerr("WARNING: nanosleep() failed: %s\n",
			       strerror(errno));
			break;
		}
	}

	sleeptimer_dequeue(next_timer);
	block_signals();
	next_timer->callback(next_timer);
	unblock_signals();

	return 0;
}
