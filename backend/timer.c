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

#include "timer.h"
#include "main.h"
#include "log.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


static LIST_HEAD(timer_list);
static unsigned int id_counter;

#if defined(__mips__) && !defined(__NR_Linux)
# define __NR_Linux		4000
#endif

#ifndef __NR_clock_gettime
# if defined(__powerpc__)
#  define __NR_clock_gettime	246
# elif defined(__i386__)
#  define __NR_clock_gettime	(259+6)
# elif defined(__x86_64__)
#  define __NR_clock_gettime	228
# elif defined(__arm__)
#  define __NR_clock_gettime	263
# elif defined(__mips__)
#  define __NR_clock_gettime	(__NR_Linux + 263)
# elif defined(__avr32__) || defined(__AVR32__)
#  define __NR_clock_gettime	216
# else
#  error "__NR_clock_gettime unknown"
# endif
#endif

#ifndef __NR_clock_nanosleep
# if defined(__powerpc__)
#  define __NR_clock_nanosleep	248
# elif defined(__i386__)
#  define __NR_clock_nanosleep	(259+8)
# elif defined(__x86_64__)
#  define __NR_clock_nanosleep	230
# elif defined(__arm__)
#  define __NR_clock_nanosleep	265
# elif defined(__mips__)
#  define __NR_clock_nanosleep	(__NR_Linux + 265)
# elif defined(__avr32__) || defined(__AVR32__)
#  define __NR_clock_nanosleep	218
# else
#  error "__NR_clock_nanosleep unknown"
# endif
#endif

static int timer_clock_gettime(clockid_t clock_id, struct timespec *t)
{
	return syscall(__NR_clock_gettime, clock_id, t);
}

static int timer_clock_nanosleep(clockid_t clock_id, int flags,
				 const struct timespec *req,
				 struct timespec *rem)
{
	return syscall(__NR_clock_nanosleep, clock_id, flags, req, rem);
}

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

	err = timer_clock_gettime(CLOCK_MONOTONIC, &timer->timeout);
	if (err) {
		logerr("WARNING: clock_gettime() failed: %s\n",
		       strerror(errno));
		return;
	}
	timespec_add_msec(&timer->timeout, msecs);
}

static void _sleeptimer_dequeue(struct sleeptimer *timer)
{
	list_del_init(&timer->list);
}

void sleeptimer_enqueue(struct sleeptimer *timer)
{
	struct sleeptimer *i;
	int inserted = 0;

	timer_lock();

	if (!list_empty(&timer->list))
		_sleeptimer_dequeue(timer);

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
	_sleeptimer_dequeue(timer);
	timer_unlock();
}

int sleeptimer_wait_next(void)
{
	struct timespec timeout;
	struct sleeptimer *timer = NULL, *new_timer;
	unsigned int timer_id = 0;
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
		err = timer_clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
					    &timeout, NULL);
		if (!err)
			break;
		if (errno == EAGAIN)
			continue;
		if (errno != EINTR) {
			logerr("WARNING: clock_nanosleep() failed: %s\n",
			       strerror(errno));
			break;
		}
	}

	timer_lock();
	if (!list_empty(&timer_list)) {
		new_timer = list_first_entry(&timer_list, struct sleeptimer, list);
		if (new_timer == timer && new_timer->id == timer_id)
			timer->callback(timer);
	}
	timer_unlock();

	return 0;
}
