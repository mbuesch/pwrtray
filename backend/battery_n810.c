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

#include "battery_n810.h"
#include "fileaccess.h"
#include "log.h"
#include "util.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>


static int battery_n810_update(struct battery *b)
{
	struct battery_n810 *bn = container_of(b, struct battery_n810, battery);
	int err, value;
	int value_changed = 0;

	err = file_read_int(bn->level_file, &value, 0);
	if (err) {
		logerr("WARNING: Failed to read battery charge status file\n");
		return -1;
	}
	if (value != bn->charge)
		value_changed = 1;
	bn->charge = value;

	if (value_changed)
		battery_notify_state_change(b);

	return 0;
}

static int battery_n810_charge_level(struct battery *b)
{
	struct battery_n810 *bn = container_of(b, struct battery_n810, battery);

	return bn->charge;
}

static void battery_n810_destroy(struct battery *b)
{
	struct battery_n810 *bn = container_of(b, struct battery_n810, battery);

	file_close(bn->level_file);
	free(bn);
}

static struct fileaccess * battery_n810_attr_open(int flags, const char *name)
{
	struct fileaccess *fd;

	fd = sysfs_file_open(flags, "/devices/platform/n810bm/%s", name);
	if (fd)
		return fd;
	fd = sysfs_file_open(flags, "/devices/platform/tahvo/tahvo-n810bm/%s", name);
	if (fd)
		return fd;
	fd = sysfs_file_open(flags, "/devices/platform/retu/retu-n810bm/%s", name);
	if (fd)
		return fd;

	return NULL;
}

static struct battery * battery_n810_probe(void)
{
	struct fileaccess *level_file;
	struct battery_n810 *bn;

	level_file = battery_n810_attr_open(O_RDONLY, "battery_level");
	if (!level_file)
		goto error;

	bn = zalloc(sizeof(*bn));
	if (!bn)
		goto err_close;

	bn->level_file = level_file;

	battery_init(&bn->battery, "n810");
	bn->battery.destroy = battery_n810_destroy;
	bn->battery.update = battery_n810_update;
	bn->battery.charge_level = battery_n810_charge_level;
	bn->battery.poll_interval = 10000;

	return &bn->battery;

err_close:
	file_close(level_file);
error:
	return NULL;
}

BATTERY_PROBE(n810, battery_n810_probe);
