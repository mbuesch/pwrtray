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

#include "devicelock_dummy.h"


static void devicelock_dummy_destroy(struct devicelock *s)
{
	struct devicelock_dummy *ld = container_of(s, struct devicelock_dummy, devicelock);

	free(ld);
}

static struct devicelock * devicelock_dummy_probe(void)
{
	struct devicelock_dummy *ld;


	ld = zalloc(sizeof(*ld));
	if (!ld)
		return NULL;

	devicelock_init(&ld->devicelock, "dummy");
	ld->devicelock.destroy = devicelock_dummy_destroy;

	return &ld->devicelock;
}

DEVICELOCK_PROBE(dummy, devicelock_dummy_probe);
