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


struct devicelock * devicelock_probe(void)
{
	struct devicelock *s;
	const struct probe *probe;

	for_each_probe(probe, devicelock) {
		logverbose("devicelock: Probing %p\n", probe);
		s = probe->func();
		if (s)
			return s;
	}

	return NULL;
}

void devicelock_destroy(struct devicelock *s)
{
	if (s)
		s->destroy(s);
}
