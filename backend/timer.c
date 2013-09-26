/*
 *   Copyright (C) 2010-2011 Michael Buesch <m@bues.ch>
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
#include "conf.h"

#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/prctl.h>


static LIST_HEAD(timer_list);
static timer_id_t id_counter;


static inline void timer_lock(void)
{
	block_signals();
}

static inline void timer_unlock(void)
{
	unblock_signals();
}

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

static inline int timespec_after(const struct timespec *a, const struct timespec *b)
{
	return timespec_bigger(a, b);
}

void sleeptimer_init(struct sleeptimer *timer,
		     const char *name,
		     sleeptimer_callback_t callback)
{
	memset(timer, 0, sizeof(*timer));
	timer->name = name;
	timer->callback = callback;
	INIT_LIST_HEAD(&timer->list);
	logverbose("timer: %s registered\n", name);
}

void sleeptimer_set_timeout_relative(struct sleeptimer *timer,
				     unsigned int msecs)
{
	int err;

	err = clock_gettime(CLOCK_MONOTONIC, &timer->timeout);
	if (err) {
		logerr("WARNING: clock_gettime() failed: %s\n",
		       strerror(errno));
		return;
	}
	timespec_add_msec(&timer->timeout, msecs);
}

static void do_sleeptimer_dequeue(struct sleeptimer *timer)
{
	list_del_init(&timer->list);
}

void sleeptimer_enqueue(struct sleeptimer *timer)
{
	struct sleeptimer *i;
	int inserted = 0;

	timer_lock();

	if (!list_empty(&timer->list))
		do_sleeptimer_dequeue(timer);

	list_for_each_entry(i, &timer_list, list) {
		if (timespec_after(&i->timeout, &timer->timeout)) {
			list_add_tail(&timer->list, &i->list);
			inserted = 1;
			break;
		}
	}
	if (!inserted)
		list_add_tail(&timer->list, &timer_list);
	timer->id = id_counter++;

	timer_unlock();
}

void sleeptimer_dequeue(struct sleeptimer *timer)
{
	timer_lock();
	do_sleeptimer_dequeue(timer);
	timer_unlock();
}

int sleeptimer_system_init(void)
{
#ifdef PR_SET_TIMERSLACK
	unsigned long slack;

	slack = config_get_int(backend.config, "SYSTEM", "event_slack", 1010);
	slack *= 1000000ul; /* To nanoseconds */
	if (prctl(PR_SET_TIMERSLACK, slack, 0, 0, 0))
		logerr("Failed to set timerslack to %lu ns. Ignoring.\n", slack);
#else
# warning "PR_SET_TIMERSLACK not available. Ignoring."
#endif
	return 0;
}

int sleeptimer_wait_next(void)
{
	struct timespec timeout;
	struct sleeptimer *timer = NULL;
	timer_id_t timer_id = 0;
	int err;

	timer_lock();
	if (!list_empty(&timer_list)) {
		timer = list_first_entry(&timer_list, struct sleeptimer, list);
		timeout = timer->timeout;
		timer_id = timer->id;
	}
	timer_unlock();
	if (!timer)
		return -ENOENT;

	while (1) {
		err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				      &timeout, NULL);
		if (!err)
			break;
		if (err == EAGAIN)
			continue;
		if (err != EINTR) {
			logerr("WARNING: clock_nanosleep() failed: %s\n",
			       strerror(err));
			break;
		}
	}

	timer_lock();
	list_for_each_entry(timer, &timer_list, list) {
		if (timer->id == timer_id) {
			do_sleeptimer_dequeue(timer);
			timer->callback(timer);
			break;
		}
	}
	timer_unlock();

	return 0;
}
