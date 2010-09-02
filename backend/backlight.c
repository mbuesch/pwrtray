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

#include "backlight.h"
#include "log.h"
#include "main.h"

#include "backlight_class.h"
#include "backlight_omapfb.h"

#include <errno.h>
#include <string.h>


static int default_min_brightness(struct backlight *b)
{
	return 0;
}

static int default_max_brightness(struct backlight *b)
{
	return 100;
}

static int default_brightness_step(struct backlight *b)
{
	return 1;
}

static int default_current_brightness(struct backlight *b)
{
	return 0;
}

static int default_set_brightness(struct backlight *b, int value)
{
	return -EOPNOTSUPP;
}

static void backlight_poll_callback(struct sleeptimer *timer)
{
	struct backlight *b = container_of(timer, struct backlight, timer);

	b->update(b);
	sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
	sleeptimer_enqueue(&b->timer);
}

void backlight_init(struct backlight *b)
{
	memset(b, 0, sizeof(*b));
	b->min_brightness = default_min_brightness;
	b->max_brightness = default_max_brightness;
	b->brightness_step = default_brightness_step;
	b->current_brightness = default_current_brightness;
	b->set_brightness = default_set_brightness;
}

static void backlight_start(struct backlight *b)
{
	if (b->poll_interval) {
		b->update(b);
		sleeptimer_init(&b->timer, backlight_poll_callback);
		sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
		sleeptimer_enqueue(&b->timer);
	}
}

struct backlight * backlight_probe(void)
{
	struct backlight *b;

	b = backlight_omapfb_probe();
	if (b)
		goto ok;
	b = backlight_class_probe();
	if (b)
		goto ok;

	logerr("Failed to find any backlight controls.\n");
	return NULL;

ok:
	backlight_start(b);
	return b;
}

void backlight_destroy(struct backlight *b)
{
	if (b)
		b->destroy(b);
}

int backlight_fill_pt_message_stat(struct backlight *b, struct pt_message *msg)
{
	msg->bl_stat.min_brightness = htonl(b->min_brightness(b));
	msg->bl_stat.max_brightness = htonl(b->max_brightness(b));
	msg->bl_stat.brightness_step = htonl(b->brightness_step(b));
	msg->bl_stat.brightness = htonl(b->current_brightness(b));

	return 0;
}

int backlight_notify_state_change(struct backlight *b)
{
	struct pt_message msg = {
		.id	= htons(PTNOTI_BL_CHANGED),
	};
	int err;

	err = backlight_fill_pt_message_stat(b, &msg);
	if (err)
		return err;
	notify_clients(&msg, PT_FLG_OK);

	return 0;
}
