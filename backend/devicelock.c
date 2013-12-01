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

#include "devicelock.h"
#include "log.h"


static void default_event(struct devicelock *s)
{
}

struct devicelock * devicelock_probe(void)
{
	struct devicelock *s;
	const struct probe *probe;

	for_each_probe(probe, devicelock) {
		s = probe->func();
		if (s) {
			logdebug("Initialized devicelock driver \"%s\"\n",
				 s->name);
			return s;
		}
	}

	logerr("Failed to find any devicelock controls.\n");
	return NULL;
}

void devicelock_destroy(struct devicelock *s)
{
	if (s)
		s->destroy(s);
}

void devicelock_init(struct devicelock *s, const char *name)
{
	memset(s, 0, sizeof(*s));
	s->name = name;
	s->event = default_event;
}
