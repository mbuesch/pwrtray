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

#include "screenlock_n810.h"
#include "util.h"
#include "log.h"
#include "main.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>


#define KB_BASEPATH	"/devices/platform/i2c_omap.2/i2c-2/2-0045"
#define TS_BASEPATH	"/devices/platform/omap2_mcspi.1/spi1.0"
#define INPUT_BASEPATH	"/devices/virtual/input"
#define GPIOSW_BASEPATH	"/devices/platform/gpio-switch"


static void screenlock_n810_toggle(struct screenlock *s)
{
	struct screenlock_n810 *sn = container_of(s, struct screenlock_n810, screenlock);
	int err, lock;

	lock = !(s->backlight->screen_is_locked(s->backlight));
	logdebug("screenlock: %s the device\n", lock ? "Locking" : "Unlocking");

	if (lock)
		autodim_may_suspend();

	/* Set backlight lock state */
	err = s->backlight->screen_lock(s->backlight, lock);
	if (err)
		logerr("Failed to set backlight lock state\n");
	/* Set touchscreen lock state */
	err = file_write_int(sn->ts_disable_file, lock, 10);
	if (err)
		logerr("Failed to write touchscreen disable file\n");
	/* Set keyboard lock state */
	err = file_write_int(sn->kb_disable_file, lock, 10);
	if (err)
		logerr("Failed to write keyboard disable file\n");

	if (!lock)
		autodim_may_resume();
}

static void screenlock_n810_event(struct screenlock *s)
{
	struct screenlock_n810 *sn = container_of(s, struct screenlock_n810, screenlock);
	char buf[32], *bufp;
	int count, switch_state;

	count = file_read_string(sn->kb_lock_state_file, buf, sizeof(buf));
	if (count <= 0)
		return;
	bufp = string_strip(buf);
	if (strcmp(bufp, "open") == 0) {
		switch_state = 0;
	} else if (strcmp(bufp, "closed") == 0) {
		switch_state = 1;
	} else {
		logerr("screenlock: Invalid state\n");
		return;
	}
	if (switch_state == sn->switch_state)
		return;
	sn->switch_state = switch_state;
	if (switch_state)
		screenlock_n810_toggle(s);
}

static void screenlock_n810_destroy(struct screenlock *s)
{
	struct screenlock_n810 *sn = container_of(s, struct screenlock_n810, screenlock);

	close(sn->kb_lock_evdev_fd);
	file_close(sn->kb_lock_state_file);
	file_close(sn->kb_disable_file);
	file_close(sn->ts_disable_file);
	free(sn);
}

struct screenlock * screenlock_n810_probe(struct backlight *bl)
{
	struct screenlock_n810 *sn;
	struct fileaccess *ts_disable = NULL;
	struct fileaccess *kb_disable = NULL;
	struct fileaccess *kb_lock_state = NULL;
	struct fileaccess *file;
	int kb_lock_evdev = -1;
	int kb_lock_evdev_fd = -1;
	struct dir_entry *dir_entry;
	LIST_HEAD(dir_entries);
	int err, count;
	unsigned int i;
	char buf[PATH_MAX + 1];

	ts_disable = sysfs_file_open(O_RDWR, TS_BASEPATH "/disable_ts");
	if (!ts_disable)
		goto err_close;
	kb_disable = sysfs_file_open(O_RDWR, KB_BASEPATH "/disable_kp");
	if (!kb_disable)
		goto err_close;
	kb_lock_state = sysfs_file_open(O_RDONLY, GPIOSW_BASEPATH "/kb_lock/state");
	if (!kb_lock_state)
		goto err_close;
	count = list_sysfs_directory(&dir_entries, INPUT_BASEPATH);
	if (count <= 0)
		goto err_close;
	list_for_each_entry(dir_entry, &dir_entries, list) {
		if (sscanf(dir_entry->name, "input%u", &i) != 1)
			continue;
		file = sysfs_file_open(O_RDONLY, "%s/%s/name",
				       INPUT_BASEPATH, dir_entry->name);
		if (!file)
			continue;
		count = file_read_string(file, buf, sizeof(buf));
		file_close(file);
		if (count <= 0)
			continue;
		if (strcmp(string_strip(buf), "kb_lock") == 0) {
			kb_lock_evdev = i;
			break;
		}
	}
	dir_entries_free(&dir_entries);
	if (kb_lock_evdev < 0)
		goto err_close;
	snprintf(buf, sizeof(buf), "/dev/input/event%u", kb_lock_evdev);
	kb_lock_evdev_fd = open(buf, O_RDONLY);
	if (kb_lock_evdev_fd < 0)
		goto err_close;
	err = fcntl(kb_lock_evdev_fd, F_SETFL, O_ASYNC | O_NONBLOCK);
	err |= fcntl(kb_lock_evdev_fd, F_SETSIG, SIGUSR2);
	err |= fcntl(kb_lock_evdev_fd, F_SETOWN, getpid());
	if (err)
		goto err_close;

	sn = zalloc(sizeof(*sn));
	if (!sn)
		goto err_close;
	sn->kb_lock_evdev_fd = kb_lock_evdev_fd;
	sn->kb_lock_state_file = kb_lock_state;
	sn->ts_disable_file = ts_disable;
	sn->kb_disable_file = kb_disable;

	sn->screenlock.event = screenlock_n810_event;
	sn->screenlock.destroy = screenlock_n810_destroy;

	logdebug("Screenlock toggle on %s\n", buf);

	return &sn->screenlock;

err_close:
	if (kb_lock_evdev_fd)
		close(kb_lock_evdev_fd);
	file_close(kb_lock_state);
	file_close(ts_disable);
	file_close(kb_disable);
	return NULL;
}
