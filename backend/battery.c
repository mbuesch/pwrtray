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

#include <stdint.h>
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
	int on_ac = b->on_ac(b);
	int min_level = b->min_level(b);
	int max_level = b->max_level(b);
	int level = b->charge_level(b);
	int range;

	if (on_ac < 0)
		return -1;
	range = max_level - min_level;
	if (range <= 0)
		return -1;
	level = max(0, level - min_level);
	if (level * 100 / range < 95)
		return 1;

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
	const struct probe *probe;

	for_each_probe(probe, battery) {
		b = probe->func();
		if (b) {
			battery_start(b);
			logdebug("Initialized battery driver \"%s\"\n",
				 b->name);
			return b;
		}
	}

	logerr("Failed to find a battery\n");
	return NULL;
}

void battery_destroy(struct battery *b)
{
	if (b)
		b->destroy(b);
}

static void battery_emergency_check(struct battery *b)
{
	int threshold, level, min_level, max_level;
	int on_ac, charging;
	const char *command;
	pid_t pid;

	threshold = config_get_int(backend.config, "BATTERY",
				   "emergency_threshold", 0);
	if (threshold <= 0)
		return;
	threshold = min(threshold, 95);

	on_ac = b->on_ac(b);
	charging = b->charging(b);
	if (on_ac < 0 && charging < 0) {
		/* We don't know whether we are charging or draining
		 * the battery. Assume charging and don't flag emergency.
		 */
		return;
	}
	if (on_ac > 0 || charging > 0) {
		/* We are not draining the battery. */
		return;
	}

	min_level = b->min_level(b);
	max_level = b->max_level(b);
	level = b->charge_level(b);
	if (min_level < 0 || max_level < 0 || level < 0)
		return;

	/* To percent */
	level = (int64_t)(level - min_level) * 100 / (max_level - min_level);

	if (level > threshold) {
		/* No emergency status */
		b->emergency_handled = 0;
		return;
	}
	if (b->emergency_handled)
		return;

	loginfo("Battery emergency threshold reached. (level=%d%% <= threshold=%d%%)\n",
		level, threshold);

	b->emergency_handled = 1;

	command = config_get(backend.config, "BATTERY",
			     "emergency_command", NULL);
	if (strempty(command)) {
		logerr("No 'emergency_command' in 'BATTERY' section of "
		       "config file. Ignoring emergency state.\n");
		return;
	}
	loginfo("Executing '%s'\n", command);

	pid = subprocess_exec(command);
	if (pid < 0) {
		logerr("Failed to execute 'emergency_command'.\n");
		return;
	}
}

int battery_fill_pt_message_stat(struct battery *b, struct pt_message *msg)
{
	int on_ac = b->on_ac(b);
	int charging = b->charging(b);

	msg->bat_stat.flags = 0;
	if (on_ac > 0)
		msg->bat_stat.flags |= htonl(PT_BAT_FLG_ONAC);
	else if (on_ac < 0)
		msg->bat_stat.flags |= htonl(PT_BAT_FLG_ACUNKNOWN);
	if (charging > 0)
		msg->bat_stat.flags |= htonl(PT_BAT_FLG_CHARGING);
	else if (charging < 0)
		msg->bat_stat.flags |= htonl(PT_BAT_FLG_CHUNKNOWN);
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

	battery_emergency_check(b);

	if (backend.autodim)
		autodim_handle_battery_event(backend.autodim);

	err = battery_fill_pt_message_stat(b, &msg);
	if (err)
		return err;
	notify_clients(&msg, PT_FLG_OK);

	return 0;
}
