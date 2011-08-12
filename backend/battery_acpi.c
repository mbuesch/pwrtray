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

#include "battery_acpi.h"
#include "util.h"
#include "fileaccess.h"
#include "log.h"

#include <limits.h>
#include <errno.h>


#define AC_BASEDIR	"/bus/acpi/drivers/ac"
#define BATT_BASEDIR	"/bus/acpi/drivers/battery"


static int battery_acpi_update(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);
	int value, err, value_changed = 0;
	struct fileaccess *file;

	file = sysfs_file_open(O_RDONLY, ba->ac_online_filename);
	if (!file) {
		logerr("WARNING: Failed to open ac/online file\n");
		return -ETXTBSY;
	}
	err = file_read_int(file, &value, 10);
	file_close(file);
	if (err) {
		logerr("WARNING: Failed to read ac/online file\n");
		return -ETXTBSY;
	}
	if (value != ba->on_ac) {
		ba->on_ac = value;
		value_changed = 1;
	}

	value = 0;
	file = sysfs_file_open(O_RDONLY, ba->charge_max_filename);
	if (file) {
		err = file_read_int(file, &value, 10);
		file_close(file);
		if (err) {
			logerr("WARNING: Failed to read charge_max file\n");
			return -ETXTBSY;
		}
	}
	if (value != ba->charge_max) {
		ba->charge_max = value;
		value_changed = 1;
	}

	value = 0;
	file = sysfs_file_open(O_RDONLY, ba->charge_now_filename);
	if (file) {
		err = file_read_int(file, &value, 10);
		file_close(file);
		if (err) {
			logerr("WARNING: Failed to read charge_now file\n");
			return -ETXTBSY;
		}
	}
	if (value != ba->charge_now) {
		ba->charge_now = value;
		value_changed = 1;
	}

	if (value_changed)
		battery_notify_state_change(b);

	return 0;
}

static int battery_acpi_on_ac(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	return ba->on_ac;
}

static int battery_acpi_max_charge(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	return ba->charge_max;
}

static int battery_acpi_current_charge(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	return ba->charge_now;
}

static void battery_acpi_destroy(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	free(ba);
}

struct battery * battery_acpi_probe(void)
{
	struct battery_acpi *ba;
	LIST_HEAD(dentries);
	struct dir_entry *dentry;
	char ac_dirname[NAME_MAX + 1] = { 0, };
	char batt_dirname[NAME_MAX + 1] = { 0, };
	int res;

	res = list_sysfs_directory(&dentries, AC_BASEDIR);
	if (res <= 0)
		goto error;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type != DT_LNK ||
		    strncmp(dentry->name, "ACPI", 4) != 0)
			continue;
		strncpy(ac_dirname, dentry->name, sizeof(ac_dirname) - 1);
		break;
	}
	dir_entries_free(&dentries);

	res = list_sysfs_directory(&dentries, BATT_BASEDIR);
	if (res <= 0)
		goto error;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type != DT_LNK ||
		    strncmp(dentry->name, "PNP", 3) != 0)
			continue;
		strncpy(batt_dirname, dentry->name, sizeof(batt_dirname) - 1);
		break;
	}
	dir_entries_free(&dentries);

	if (ac_dirname[0] == 0 || batt_dirname[0] == 0)
		goto error;

	ba = zalloc(sizeof(*ba));
	if (!ba)
		goto error;

	battery_init(&ba->battery, "acpi");
	ba->battery.destroy = battery_acpi_destroy;
	ba->battery.update = battery_acpi_update;
	ba->battery.on_ac = battery_acpi_on_ac;
	ba->battery.max_charge = battery_acpi_max_charge;
	ba->battery.current_charge = battery_acpi_current_charge;
	ba->battery.poll_interval = 10000;
	snprintf(ba->ac_online_filename, sizeof(ba->ac_online_filename),
		 "%s/%s/power_supply/AC0/online",
		 AC_BASEDIR, ac_dirname);
	snprintf(ba->charge_max_filename, sizeof(ba->charge_max_filename),
		 "%s/%s/power_supply/BAT0/charge_full",
		 BATT_BASEDIR, batt_dirname);
	snprintf(ba->charge_now_filename, sizeof(ba->charge_now_filename),
		 "%s/%s/power_supply/BAT0/charge_now",
		 BATT_BASEDIR, batt_dirname);

	return &ba->battery;

error:
	return NULL;
}
