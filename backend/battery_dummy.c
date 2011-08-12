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

#include "battery_dummy.h"
#include "util.h"


static int battery_dummy_max_charge(struct battery *b)
{
	return 0;
}

static void battery_dummy_destroy(struct battery *b)
{
	struct battery_dummy *bd = container_of(b, struct battery_dummy, battery);

	free(bd);
}

struct battery * battery_dummy_probe(void)
{
	struct battery_dummy *bd;

	bd = zalloc(sizeof(*bd));
	if (!bd)
		return NULL;

	battery_init(&bd->battery, "dummy");
	bd->battery.destroy = battery_dummy_destroy;
	bd->battery.max_charge = battery_dummy_max_charge;

	return &bd->battery;
}
