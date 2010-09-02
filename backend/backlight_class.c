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

#include "backlight_class.h"
#include "fileaccess.h"
#include "log.h"
#include "util.h"

#include <string.h>
#include <limits.h>
#include <errno.h>


#define BASEPATH	"/class/backlight"


static int backlight_class_max_brightness(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);

	return bc->max_brightness;
}

static int backlight_class_current_brightness(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);

	return bc->brightness;
}

static int backlight_class_set_brightness(struct backlight *b, int value)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);
	int err;

	value = min(b->max_brightness(b), value);
	value = max(b->min_brightness(b), value);
	err = file_write_int(bc->set_br_file, value, 10);
	if (err)
		return err;
	bc->brightness = value;

	return b->update(b);
}

static int backlight_class_update(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);
	int err, value;
	int value_changed = 0;

	err = file_read_int(bc->actual_br_file, &value, 0);
	if (err) {
		logerr("WARNING: Failed to read actual backlight brightness\n");
		return err;
	}
	if (value != bc->brightness)
		value_changed = 1;
	bc->brightness = value;

	if (value_changed)
		backlight_notify_state_change(b);

	return 0;
}

static void backlight_class_destroy(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);

	file_close(bc->actual_br_file);
	file_close(bc->set_br_file);

	free(bc);
}

struct backlight * backlight_class_probe(void)
{
	struct backlight_class *bc;
	struct fileaccess *file, *actual_br_file = NULL, *set_br_file = NULL;
	LIST_HEAD(dir_entries);
	struct dir_entry *dir_entry;
	int err, max_brightness;

	err = list_sysfs_directory(&dir_entries, BASEPATH);
	if (err)
		return NULL;
	if (list_empty(&dir_entries))
		return NULL;
	dir_entry = list_first_entry(&dir_entries, struct dir_entry, list);

	file = sysfs_file_open(O_RDONLY, "%s/%s/max_brightness", BASEPATH, dir_entry->name);
	if (!file)
		goto err_files_close;
	err = file_read_int(file, &max_brightness, 0);
	file_close(file);
	if (err)
		goto err_files_close;

	actual_br_file = sysfs_file_open(O_RDONLY, "%s/%s/actual_brightness", BASEPATH, dir_entry->name);
	if (!actual_br_file)
		goto err_files_close;
	set_br_file = sysfs_file_open(O_RDWR, "%s/%s/brightness", BASEPATH, dir_entry->name);
	if (!set_br_file)
		goto err_files_close;

	bc = malloc(sizeof(*bc));
	if (!bc)
		goto err_files_close;
	memset(bc, 0, sizeof(*bc));

	bc->actual_br_file = actual_br_file;
	bc->set_br_file = set_br_file;
	bc->max_brightness = max_brightness;

	backlight_init(&bc->backlight);
	bc->backlight.max_brightness = backlight_class_max_brightness;
	bc->backlight.current_brightness = backlight_class_current_brightness;
	bc->backlight.set_brightness = backlight_class_set_brightness;
	bc->backlight.destroy = backlight_class_destroy;
	bc->backlight.update = backlight_class_update;
	bc->backlight.poll_interval = 0;

	err = bc->backlight.update(&bc->backlight);
	if (err)
		goto err_free;

	dir_entries_free(&dir_entries);

	return &bc->backlight;

err_free:
	free(bc);
err_files_close:
	file_close(set_br_file);
	file_close(actual_br_file);
	dir_entries_free(&dir_entries);

	return NULL;
}
