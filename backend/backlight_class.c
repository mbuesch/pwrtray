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
	if (bc->brightness == value)
		return 0;
	err = file_write_int(bc->set_br_file, value, 10);
	if (err)
		return err;
	bc->brightness = value;
	backlight_notify_state_change(b);

	return 0;
}

static int backlight_class_read_file(struct backlight_class *bc)
{
	int err, value;

	err = file_read_int(bc->actual_br_file, &value, 0);
	if (err) {
		logerr("WARNING: Failed to read actual backlight brightness\n");
		return -abs(err);
	}

	return value;
}

static int backlight_class_update(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);
	int expected_brightness, cur_brightness, err, res;

	expected_brightness = bc->brightness;
	res = backlight_class_read_file(bc);
	if (res < 0)
		return res;
	cur_brightness = res;

	if (cur_brightness != expected_brightness &&
	    cur_brightness != b->min_brightness(b)) {
		logdebug("Amending backlight brightness from %d to %d\n",
			 cur_brightness, expected_brightness);
		err = backlight_class_set_brightness(b, expected_brightness);
		if (err)
			return err;
	}

	return 0;
}

static void backlight_class_destroy(struct backlight *b)
{
	struct backlight_class *bc = container_of(b, struct backlight_class, backlight);

	file_close(bc->actual_br_file);
	file_close(bc->set_br_file);

	free(bc);
}

static char * backlight_select_sysfs_dir(struct list_head *dir_entries)
{
	struct dir_entry *ent;

	/* Prefer acpi_video. */
	list_for_each_entry(ent, dir_entries, list) {
		if (!(ent->type & (DT_LNK | DT_DIR)))
			continue;
		if (strncmp(ent->name, "acpi_video", 10) == 0)
			return ent->name;
	}

	/* Return first entry, otherwise. */
	list_for_each_entry(ent, dir_entries, list) {
		if (!(ent->type & (DT_LNK | DT_DIR)))
			continue;
		return ent->name;
	}

	return NULL;
}

static struct backlight * backlight_class_probe(void)
{
	struct backlight_class *bc;
	struct fileaccess *file, *actual_br_file = NULL, *set_br_file = NULL;
	LIST_HEAD(dir_entries);
	int err, res, max_brightness;
	const char *dirname;

	err = list_sysfs_directory(&dir_entries, BASEPATH);
	if (err <= 0)
		return NULL;
	dirname = backlight_select_sysfs_dir(&dir_entries);
	if (!dirname)
		goto err_ent_free;
	logdebug("class backlight: Using '%s' for brightness adjustment\n",
		 dirname);

	file = sysfs_file_open(O_RDONLY, "%s/%s/max_brightness",
			       BASEPATH, dirname);
	if (!file)
		goto err_files_close;
	err = file_read_int(file, &max_brightness, 0);
	file_close(file);
	if (err)
		goto err_files_close;

	actual_br_file = sysfs_file_open(O_RDONLY, "%s/%s/actual_brightness",
					 BASEPATH, dirname);
	if (!actual_br_file)
		goto err_files_close;
	set_br_file = sysfs_file_open(O_RDWR, "%s/%s/brightness",
				      BASEPATH, dirname);
	if (!set_br_file)
		goto err_files_close;

	bc = zalloc(sizeof(*bc));
	if (!bc)
		goto err_files_close;

	bc->actual_br_file = actual_br_file;
	bc->set_br_file = set_br_file;
	bc->max_brightness = max_brightness;

	backlight_init(&bc->backlight, "class");
	bc->backlight.max_brightness = backlight_class_max_brightness;
	bc->backlight.current_brightness = backlight_class_current_brightness;
	bc->backlight.set_brightness = backlight_class_set_brightness;
	bc->backlight.destroy = backlight_class_destroy;
	bc->backlight.update = backlight_class_update;
	bc->backlight.poll_interval = 2000;

	res = backlight_class_read_file(bc);
	if (res < 0)
		goto err_free;
	bc->brightness = res;
	backlight_notify_state_change(&bc->backlight);

	dir_entries_free(&dir_entries);

	return &bc->backlight;

err_free:
	free(bc);
err_files_close:
	file_close(set_br_file);
	file_close(actual_br_file);
err_ent_free:
	dir_entries_free(&dir_entries);

	return NULL;
}

BACKLIGHT_PROBE(class, backlight_class_probe);
