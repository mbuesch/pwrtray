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

static int battery_acpi_max_level(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	return ba->charge_max;
}

static int battery_acpi_charge_level(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	return ba->charge_now;
}

static void battery_acpi_destroy(struct battery *b)
{
	struct battery_acpi *ba = container_of(b, struct battery_acpi, battery);

	free(ba->ac_online_filename);
	free(ba->charge_max_filename);
	free(ba->charge_now_filename);

	free(ba);
}

static struct battery * battery_acpi_probe(void)
{
	struct battery_acpi *ba;
	LIST_HEAD(dentries);
	struct dir_entry *dentry;
	char ac_file[PATH_MAX + 1] = { 0, };
	char full_file[PATH_MAX + 1] = { 0, };
	char now_file[PATH_MAX + 1] = { 0, };
	int res, ok;

	/* Init base directories. */
	strncat(ac_file, AC_BASEDIR, sizeof(ac_file) - strlen(ac_file) - 1);
	strncat(full_file, BATT_BASEDIR, sizeof(full_file) - strlen(full_file) - 1);
	strncat(now_file, BATT_BASEDIR, sizeof(now_file) - strlen(now_file) - 1);

	/* Find the AC-online file */
	res = list_sysfs_directory(&dentries, ac_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_LNK &&
		    strncmp(dentry->name, "ACPI", 4) == 0) {
			strncat(ac_file, "/", sizeof(ac_file) - strlen(ac_file) - 1);
			strncat(ac_file, dentry->name, sizeof(ac_file) - strlen(ac_file) - 1);
			strncat(ac_file, "/power_supply", sizeof(ac_file) - strlen(ac_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;
	res = list_sysfs_directory(&dentries, ac_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_DIR &&
		    strncmp(dentry->name, "AC", 2) == 0) {
			strncat(ac_file, "/", sizeof(ac_file) - strlen(ac_file) - 1);
			strncat(ac_file, dentry->name, sizeof(ac_file) - strlen(ac_file) - 1);
			strncat(ac_file, "/online", sizeof(ac_file) - strlen(ac_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	/* Find the battery-full file */
	res = list_sysfs_directory(&dentries, full_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_LNK &&
		    strncmp(dentry->name, "PNP", 3) == 0) {
			strncat(full_file, "/", sizeof(full_file) - strlen(full_file) - 1);
			strncat(full_file, dentry->name, sizeof(full_file) - strlen(full_file) - 1);
			strncat(full_file, "/power_supply", sizeof(full_file) - strlen(full_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;
	res = list_sysfs_directory(&dentries, full_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_DIR &&
		    strncmp(dentry->name, "BAT", 3) == 0) {
			strncat(full_file, "/", sizeof(full_file) - strlen(full_file) - 1);
			strncat(full_file, dentry->name, sizeof(full_file) - strlen(full_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;
	res = list_sysfs_directory(&dentries, full_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_REG &&
		    (strcmp(dentry->name, "charge_full") == 0 ||
		     strcmp(dentry->name, "energy_full") == 0)) {
			strncat(full_file, "/", sizeof(full_file) - strlen(full_file) - 1);
			strncat(full_file, dentry->name, sizeof(full_file) - strlen(full_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	/* Find the battery-now file */
	res = list_sysfs_directory(&dentries, now_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_LNK &&
		    strncmp(dentry->name, "PNP", 3) == 0) {
			strncat(now_file, "/", sizeof(now_file) - strlen(now_file) - 1);
			strncat(now_file, dentry->name, sizeof(now_file) - strlen(now_file) - 1);
			strncat(now_file, "/power_supply", sizeof(now_file) - strlen(now_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;
	res = list_sysfs_directory(&dentries, now_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_DIR &&
		    strncmp(dentry->name, "BAT", 3) == 0) {
			strncat(now_file, "/", sizeof(now_file) - strlen(now_file) - 1);
			strncat(now_file, dentry->name, sizeof(now_file) - strlen(now_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;
	res = list_sysfs_directory(&dentries, now_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if (dentry->type == DT_REG &&
		    (strcmp(dentry->name, "charge_now") == 0 ||
		     strcmp(dentry->name, "energy_now") == 0)) {
			strncat(now_file, "/", sizeof(now_file) - strlen(now_file) - 1);
			strncat(now_file, dentry->name, sizeof(now_file) - strlen(now_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	ba = zalloc(sizeof(*ba));
	if (!ba)
		goto error;

	battery_init(&ba->battery, "acpi");
	ba->battery.destroy = battery_acpi_destroy;
	ba->battery.update = battery_acpi_update;
	ba->battery.on_ac = battery_acpi_on_ac;
	ba->battery.max_level = battery_acpi_max_level;
	ba->battery.charge_level = battery_acpi_charge_level;
	ba->battery.poll_interval = 10000;
	ba->ac_online_filename = strdup(ac_file);
	ba->charge_max_filename = strdup(full_file);
	ba->charge_now_filename = strdup(now_file);

	return &ba->battery;

error:
	return NULL;
}

BATTERY_PROBE(acpi, battery_acpi_probe);
