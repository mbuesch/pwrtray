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

#include "screenlock.h"

#include "screenlock_n810.h"


struct screenlock * screenlock_probe(struct backlight *bl)
{
	struct screenlock *s;

	s = screenlock_n810_probe(bl);
	if (s)
		goto ok;

	return NULL;

ok:
	s->backlight = bl;

	return s;
}

void screenlock_destroy(struct screenlock *s)
{
	if (s)
		s->destroy(s);
}
