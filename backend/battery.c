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

#include "battery.h"
#include "log.h"
#include "main.h"

#include "battery_powerbook.h"
#include "battery_n810.h"
#include "battery_dummy.h"

#include <string.h>
#include <stdio.h>


static int default_on_ac(struct battery *b)
{
	return 0;
}

static int default_min_charge(struct battery *b)
{
	return 0;
}

static int default_max_charge(struct battery *b)
{
	return 100;
}

static int default_current_charge(struct battery *b)
{
	return 0;
}

static void battery_poll_callback(struct sleeptimer *timer)
{
	struct battery *b = container_of(timer, struct battery, timer);

	b->update(b);
	sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
	sleeptimer_enqueue(&b->timer);
}

void battery_init(struct battery *b)
{
	memset(b, 0, sizeof(*b));
	b->on_ac = default_on_ac;
	b->min_charge = default_min_charge;
	b->max_charge = default_max_charge;
	b->current_charge = default_current_charge;
}

static void battery_start(struct battery *b)
{
	if (b->poll_interval) {
		b->update(b);
		sleeptimer_init(&b->timer, battery_poll_callback);
		sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
		sleeptimer_enqueue(&b->timer);
	}
}

struct battery * battery_probe(void)
{
	struct battery *b;

	b = battery_n810_probe();
	if (b)
		goto ok;
	b = battery_powerbook_probe();
	if (b)
		goto ok;
	b = battery_dummy_probe();
	if (b)
		goto ok;

	logerr("Failed to find a battery\n");
	return NULL;

ok:
	battery_start(b);
	return b;
}

void battery_destroy(struct battery *b)
{
	if (b)
		b->destroy(b);
}

int battery_fill_pt_message_stat(struct battery *b, struct pt_message *msg)
{
	msg->bat_stat.flags = 0;
	if (b->on_ac(b))
		msg->bat_stat.flags |= htonl(PT_BAT_FLG_ONAC);
	msg->bat_stat.min_charge = htonl(b->min_charge(b));
	msg->bat_stat.max_charge = htonl(b->max_charge(b));
	msg->bat_stat.charge = htonl(b->current_charge(b));

	return 0;
}

int battery_notify_state_change(struct battery *b)
{
	struct pt_message msg = {
		.id	= htons(PTNOTI_BAT_CHANGED),
	};
	int err;

	err = battery_fill_pt_message_stat(b, &msg);
	if (err)
		return err;
	notify_clients(&msg, PT_FLG_OK);

	return 0;
}
