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

#include "battery_class.h"
#include "util.h"
#include "fileaccess.h"
#include "log.h"

#include <limits.h>
#include <errno.h>


#define BASEPATH	"/class/power_supply"


static int battery_class_update(struct battery *b)
{
	struct battery_class *ba = container_of(b, struct battery_class, battery);
	int value, err, value_changed = 0;
	struct fileaccess *file;

	if (strempty(ba->ac_online_filename)) {
		value = -1;
	} else {
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
	value = min(value, ba->charge_max);
	if (value != ba->charge_now) {
		ba->charge_now = value;
		value_changed = 1;
	}

	if (value_changed)
		battery_notify_state_change(b);

	return 0;
}

static int battery_class_on_ac(struct battery *b)
{
	struct battery_class *ba = container_of(b, struct battery_class, battery);

	return ba->on_ac;
}

static int battery_class_max_level(struct battery *b)
{
	struct battery_class *ba = container_of(b, struct battery_class, battery);

	return ba->charge_max;
}

static int battery_class_charge_level(struct battery *b)
{
	struct battery_class *ba = container_of(b, struct battery_class, battery);

	return ba->charge_now;
}

static void battery_class_destroy(struct battery *b)
{
	struct battery_class *ba = container_of(b, struct battery_class, battery);

	free(ba->ac_online_filename);
	free(ba->charge_max_filename);
	free(ba->charge_now_filename);

	free(ba);
}

static void find_ac_file(char *ac_file, size_t ac_file_size)
{
	LIST_HEAD(dentries);
	struct dir_entry *dentry;
	int res, ok;

	strncat(ac_file, BASEPATH, ac_file_size - strlen(ac_file) - 1);

	/* Find the AC-online file */
	res = list_sysfs_directory(&dentries, ac_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if ((dentry->type == DT_DIR ||
		     dentry->type == DT_LNK) &&
		    strncmp(dentry->name, "AC", 2) == 0) {
			strncat(ac_file, "/", ac_file_size - strlen(ac_file) - 1);
			strncat(ac_file, dentry->name, ac_file_size - strlen(ac_file) - 1);
			strncat(ac_file, "/online", ac_file_size - strlen(ac_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	return;
error:
	ac_file[0] = '\0';
}

static void find_battery_full_file(char *full_file, size_t full_file_size)
{
	LIST_HEAD(dentries);
	struct dir_entry *dentry;
	int res, ok;

	strncat(full_file, BASEPATH, full_file_size - strlen(full_file) - 1);

	/* Find the battery-full file */
	res = list_sysfs_directory(&dentries, full_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if ((dentry->type == DT_DIR ||
		     dentry->type == DT_LNK) &&
		    strncmp(dentry->name, "BAT", 3) == 0) {
			strncat(full_file, "/", full_file_size - strlen(full_file) - 1);
			strncat(full_file, dentry->name, full_file_size - strlen(full_file) - 1);
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
			strncat(full_file, "/", full_file_size - strlen(full_file) - 1);
			strncat(full_file, dentry->name, full_file_size - strlen(full_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	return;
error:
	full_file[0] = '\0';
}

static void find_battery_now_file(char *now_file, size_t now_file_size)
{
	LIST_HEAD(dentries);
	struct dir_entry *dentry;
	int res, ok;

	strncat(now_file, BASEPATH, now_file_size - strlen(now_file) - 1);

	/* Find the battery-now file */
	res = list_sysfs_directory(&dentries, now_file);
	if (res <= 0)
		goto error;
	ok = 0;
	list_for_each_entry(dentry, &dentries, list) {
		if ((dentry->type == DT_DIR ||
		     dentry->type == DT_LNK) &&
		    strncmp(dentry->name, "BAT", 3) == 0) {
			strncat(now_file, "/", now_file_size - strlen(now_file) - 1);
			strncat(now_file, dentry->name, now_file_size - strlen(now_file) - 1);
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
			strncat(now_file, "/", now_file_size - strlen(now_file) - 1);
			strncat(now_file, dentry->name, now_file_size - strlen(now_file) - 1);
			ok = 1;
			break;
		}
	}
	dir_entries_free(&dentries);
	if (!ok)
		goto error;

	return;
error:
	now_file[0] = '\0';
}

static struct battery * battery_class_probe(void)
{
	struct battery_class *ba;
	char ac_file[PATH_MAX + 1] = { 0, };
	char full_file[PATH_MAX + 1] = { 0, };
	char now_file[PATH_MAX + 1] = { 0, };

	find_ac_file(ac_file, sizeof(ac_file));
	find_battery_full_file(full_file, sizeof(full_file));
	find_battery_now_file(now_file, sizeof(now_file));
	if (strempty(full_file) || strempty(now_file))
		goto error;

	ba = zalloc(sizeof(*ba));
	if (!ba)
		goto error;

	battery_init(&ba->battery, "class");
	ba->battery.destroy = battery_class_destroy;
	ba->battery.update = battery_class_update;
	ba->battery.on_ac = battery_class_on_ac;
	ba->battery.max_level = battery_class_max_level;
	ba->battery.charge_level = battery_class_charge_level;
	ba->battery.poll_interval = 10000;
	ba->ac_online_filename = strdup(ac_file);
	ba->charge_max_filename = strdup(full_file);
	ba->charge_now_filename = strdup(now_file);

	return &ba->battery;

error:
	return NULL;
}

BATTERY_PROBE(class, battery_class_probe);
