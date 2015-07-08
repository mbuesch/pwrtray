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

#include "backlight.h"
#include "log.h"
#include "main.h"
#include "x11lock.h"
#include "util.h"

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/fb.h>


static int do_framebuffer_blank(int fd, int blank)
{
	int ret;
	unsigned long arg;

	arg = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
	ret = ioctl(fd, FBIOBLANK, arg);
	if (ret)
		return -ENODEV;
	logdebug("Framebuffer %s\n", blank ? "blanked" : "unblanked");

	return 0;
}

static int framebuffer_blank(struct backlight *b, int blank)
{
	int err;

	if (b->framebuffer_fd < 0)
		return 0;
	blank = !!blank;
	if (blank == b->fb_blanked)
		return 0;
	b->fb_blanked = blank;

	err = do_framebuffer_blank(b->framebuffer_fd, blank);
	if (err) {
		logerr("Failed to %s the fb device (err %d)\n",
		       blank ? "blank" : "unblank", err);
	}

	return err;
}

static void fbblank_exit(struct backlight *b)
{
	if (b->framebuffer_fd >= 0) {
		do_framebuffer_blank(b->framebuffer_fd, 0);
		close(b->framebuffer_fd);
		b->framebuffer_fd = -1;
	}
}

static void fbblank_init(struct backlight *b)
{
	const char *fbdev;
	int err, fd;

	b->framebuffer_fd = -1;

	fbdev = config_get(backend.config, "SCREEN", "fbdev", NULL);
	if (!fbdev)
		return;

	fd = open(fbdev, O_RDWR);
	if (fd < 0) {
		logerr("Failed to open framebuffer dev %s: %s\n",
		       fbdev, strerror(errno));
		return;
	}
	err = do_framebuffer_blank(fd, 0);
	if (err) {
		close(fd);
		return;
	}
	b->fb_blanked = 0;
	b->framebuffer_fd = fd;

	logdebug("Opened %s for fb blanking\n", fbdev);
}

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

static int default_screen_lock(struct backlight *b, int lock)
{
	return -EOPNOTSUPP;
}

static int default_screen_is_locked(struct backlight *b)
{
	return 0;
}

static void backlight_poll_callback(struct sleeptimer *timer)
{
	struct backlight *b = container_of(timer, struct backlight, timer);

	b->update(b);
	sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
	sleeptimer_enqueue(&b->timer);
}

void backlight_init(struct backlight *b, const char *name)
{
	memset(b, 0, sizeof(*b));
	b->name = name;
	b->min_brightness = default_min_brightness;
	b->max_brightness = default_max_brightness;
	b->brightness_step = default_brightness_step;
	b->current_brightness = default_current_brightness;
	b->set_brightness = default_set_brightness;
	b->screen_lock = default_screen_lock;
	b->screen_is_locked = default_screen_is_locked;
}

static void backlight_start(struct backlight *b)
{
	if (b->poll_interval) {
		b->update(b);
		sleeptimer_init(&b->timer, "backlight", backlight_poll_callback);
		sleeptimer_set_timeout_relative(&b->timer, b->poll_interval);
		sleeptimer_enqueue(&b->timer);
	}
}

struct backlight * backlight_probe(void)
{
	struct backlight *b;
	const struct probe *probe;

	for_each_probe(probe, backlight) {
		b = probe->func();
		if (b) {
			fbblank_init(b);
			backlight_start(b);
			logdebug("Initialized backlight driver \"%s\"\n",
				 b->name);
			return b;
		}
	}

	logerr("Failed to find any backlight controls.\n");
	return NULL;
}

void backlight_destroy(struct backlight *b)
{
	if (!b)
		return;

	fbblank_exit(b);
	b->screen_lock(b, 0);
	b->set_brightness(b, b->max_brightness(b));
	b->destroy(b);
}

int backlight_fill_pt_message_stat(struct backlight *b, struct pt_message *msg)
{
	int min_brightness, max_brightness, cur_brightness, brightness_step;
	int autodim_enabled, autodim_ac;

	min_brightness = b->min_brightness(b);
	max_brightness = b->max_brightness(b);
	cur_brightness = b->current_brightness(b);
	brightness_step = b->brightness_step(b);

	autodim_enabled = b->autodim_enabled;
	autodim_ac = b->autodim_enabled_on_ac;

	if (autodim_enabled) {
		cur_brightness = backlight_get_percentage(b);
		if (cur_brightness < 0)
			cur_brightness = 100;
		min_brightness = 0;
		max_brightness = 100;
		brightness_step = 1;
	}

	msg->bl_stat.min_brightness = htonl(min_brightness);
	msg->bl_stat.max_brightness = htonl(max_brightness);
	msg->bl_stat.brightness_step = htonl(brightness_step);
	msg->bl_stat.brightness = htonl(cur_brightness);
	msg->bl_stat.flags = autodim_enabled ? htonl(PT_BL_FLG_AUTODIM) : 0;
	msg->bl_stat.flags |= autodim_ac ? htonl(PT_BL_FLG_AUTODIM_AC) : 0;

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

static void update_screen_blanking(struct backlight *b)
{
	int brightness = b->current_brightness(b);
	int screen_locked = b->screen_is_locked(b);

	if (screen_locked || brightness == 0) {
		if (screen_locked)
			block_x11_input(&backend.x11lock);
		framebuffer_blank(b, 1);
	} else {
		framebuffer_blank(b, 0);
		unblock_x11_input(&backend.x11lock);
	}
}

int backlight_set_brightness(struct backlight *b, int value)
{
	int err;

	err = b->set_brightness(b, value);
	if (err)
		return err;
	update_screen_blanking(b);

	return 0;
}

int backlight_screen_lock(struct backlight *b, int lock)
{
	int err;

	err = b->screen_lock(b, lock);
	if (err)
		return err;
	update_screen_blanking(b);

	return 0;
}

int backlight_set_percentage(struct backlight *b, unsigned int percent)
{
	int bmin = b->min_brightness(b);
	int bmax = b->max_brightness(b);
	int err, value, range;

	range = bmax - bmin;
	value = div_round((int64_t)range * percent, (int64_t)100) + bmin;

	err = backlight_set_brightness(b, value);
	if (!err)
		backlight_notify_state_change(b);

	return err;
}

int backlight_get_percentage(struct backlight *b)
{
	int bmin = b->min_brightness(b);
	int bmax = b->max_brightness(b);
	int percent, value, range;

	range = bmax - bmin;
	value = b->current_brightness(b);
	if (value < 0)
		return value;
	value -= bmin;
	if (value < 0)
		return -EINVAL;

	percent = div_round((int64_t)value * 100, (int64_t)range);
	percent = clamp(percent, 0, 100);

	return percent;
}
