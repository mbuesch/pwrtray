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

#include "devicelock_n810.h"


struct devicelock * devicelock_probe(void)
{
	struct devicelock *s;

	s = devicelock_n810_probe();
	if (s)
		goto ok;

	return NULL;

ok:
	return s;
}

void devicelock_destroy(struct devicelock *s)
{
	if (s)
		s->destroy(s);
}
