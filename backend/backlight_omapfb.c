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

#include "backlight_omapfb.h"
#include "fileaccess.h"
#include "log.h"
#include "util.h"

#include <string.h>


#define BASEPATH	"/devices/platform/omapfb/panel"


static int omapfb_write_brightness(struct backlight_omapfb *bo)
{
	int err, level;

	if (bo->locked)
		level = 0;
	else
		level = bo->current_level;

	err = file_write_int(bo->level_file, level, 10);
	if (err)
		return err;
	if (level == 0) {
		//TODO disable screen
	}
}

static int backlight_omapfb_max_brightness(struct backlight *b)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);

	return bo->max_level;
}

static int backlight_omapfb_current_brightness(struct backlight *b)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);

	return bo->current_level;
}

static int backlight_omapfb_set_brightness(struct backlight *b, int value)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);
	int err;

	value = min(b->max_brightness(b), value);
	value = max(b->min_brightness(b), value);
	bo->current_level = value;
	err = omapfb_write_brightness(bo);
	if (err)
		return err;

	return b->update(b);
}

static int backlight_omapfb_screen_lock(struct backlight *b, int lock)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);
	int err;

	lock = !!lock;
	if (lock == bo->locked)
		return 0;
	bo->locked = lock;
	err = omapfb_write_brightness(bo);
	if (err)
		return err;
	backlight_notify_state_change(b);

	return 0;
}

static int backlight_omapfb_screen_is_locked(struct backlight *b)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);

	return bo->locked;
}

static int backlight_omapfb_update(struct backlight *b)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);
	int err, value;
	int value_changed = 0;

	err = file_read_int(bo->level_file, &value, 0);
	if (err) {
		logerr("WARNING: Failed to read backlight level file\n");
		return err;
	}
	if (value != bo->current_level)
		value_changed = 1;
	bo->current_level = value;

	if (value_changed)
		backlight_notify_state_change(b);

	return 0;
}

static void backlight_omapfb_destroy(struct backlight *b)
{
	struct backlight_omapfb *bo = container_of(b, struct backlight_omapfb, backlight);

	file_close(bo->level_file);
	free(bo);
}

struct backlight * backlight_omapfb_probe(void)
{
	struct backlight_omapfb *bo;
	struct fileaccess *file, *level_file;
	int max_level;
	int err;

	file = sysfs_file_open(O_RDONLY, "%s/backlight_max", BASEPATH);
	if (!file)
		goto error;
	err = file_read_int(file, &max_level, 0);
	file_close(file);
	if (err)
		goto error;
	level_file = sysfs_file_open(O_RDWR, "%s/backlight_level", BASEPATH);
	if (!level_file)
		goto error;

	bo = zalloc(sizeof(*bo));
	if (!bo)
		goto err_close;

	bo->level_file = level_file;
	bo->max_level = max_level;

	backlight_init(&bo->backlight);
	bo->backlight.max_brightness = backlight_omapfb_max_brightness;
	bo->backlight.current_brightness = backlight_omapfb_current_brightness;
	bo->backlight.set_brightness = backlight_omapfb_set_brightness;
	bo->backlight.screen_lock = backlight_omapfb_screen_lock;
	bo->backlight.screen_is_locked = backlight_omapfb_screen_is_locked;
	bo->backlight.destroy = backlight_omapfb_destroy;
	bo->backlight.update = backlight_omapfb_update;
	bo->backlight.poll_interval = 0;

	err = bo->backlight.update(&bo->backlight);
	if (err)
		goto err_free;

	return &bo->backlight;

err_free:
	free(bo);
err_close:
	file_close(level_file);
error:
	return NULL;
}
