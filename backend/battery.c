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

#include "battery.h"
#include "log.h"
#include "main.h"

#include "battery_powerbook.h"
#include "battery_n810.h"
#include "battery_dummy.h"
#include "battery_acpi.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>


static int default_on_ac(struct battery *b)
{
	return 0;
}

static int default_charger_enable(struct battery *b, int enable)
{
	return -ENODEV;
}

static int default_charging(struct battery *b)
{
	return 0;
}

static int default_min_level(struct battery *b)
{
	return 0;
}

static int default_max_level(struct battery *b)
{
	return 100;
}

static int default_charge_level(struct battery *b)
{
	return 0;
}

static int default_capacity_mAh(struct battery *b)
{
	return -ENODEV;
}

static int default_current_mA(struct battery *b)
{
	return -ENODEV;
}

static int default_temperature_K(struct battery *b)
{
	return -ENODEV;
}

static void battery_poll_callback(struct sleeptimer *timer)
{
	struct battery *b = container_of(timer, struct battery, timer);

	b->update(b);
	sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
	sleeptimer_enqueue(&b->timer);
}

void battery_init(struct battery *b, const char *name)
{
	memset(b, 0, sizeof(*b));
	b->name = name;
	b->on_ac = default_on_ac;
	b->charger_enable = default_charger_enable;
	b->charging = default_charging;
	b->min_level = default_min_level;
	b->max_level = default_max_level;
	b->charge_level = default_charge_level;
	b->capacity_mAh = default_capacity_mAh;
	b->current_mA = default_current_mA;
	b->temperature_K = default_temperature_K;
}

static void battery_start(struct battery *b)
{
	if (b->poll_interval) {
		b->update(b);
		sleeptimer_init(&b->timer, "battery", battery_poll_callback);
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
	b = battery_acpi_probe();
	if (b)
		goto ok;
	b = battery_dummy_probe();
	if (b)
		goto ok;

	logerr("Failed to find a battery\n");
	return NULL;

ok:
	battery_start(b);
	logdebug("Initialied battery driver \"%s\"\n", b->name);

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
	msg->bat_stat.min_level = htonl(b->min_level(b));
	msg->bat_stat.max_level = htonl(b->max_level(b));
	msg->bat_stat.level = htonl(b->charge_level(b));

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
