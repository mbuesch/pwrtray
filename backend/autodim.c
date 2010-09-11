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

#include "autodim.h"
#include "log.h"
#include "fileaccess.h"
#include "util.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>


#define AUTODIM_TIMER_INTERVAL	1000


static void autodim_set_backlight(struct autodim *ad, unsigned int percent)
{
	if (percent != ad->bl_percent) {
		ad->bl_percent = percent;
		backlight_set_percentage(ad->bl, percent);
		logverbose("Autodim: Set backlight to %u percent.\n", percent);
	}
}

static void autodim_handle_state(struct autodim *ad)
{
	if (ad->state <= 5) {
		autodim_set_backlight(ad, ad->max_percent * 100 / 100);
	} else if (ad->state <= 15) {
		autodim_set_backlight(ad, ad->max_percent * 75 / 100);
	} else if (ad->state <= 30) {
		autodim_set_backlight(ad, ad->max_percent * 50 / 100);
	} else if (ad->state <= 60) {
		autodim_set_backlight(ad, ad->max_percent * 25 / 100);
	} else {
		if (ad->dim_fully_dark)
			autodim_set_backlight(ad, 0);
	}
}

static void autodim_timer_callback(struct sleeptimer *timer)
{
	struct autodim *ad = container_of(timer, struct autodim, timer);

	if (ad->state < 0xFFFFFFFF) {
		ad->state++;
		autodim_handle_state(ad);
	}

	sleeptimer_set_timeout_relative(timer, AUTODIM_TIMER_INTERVAL);
	sleeptimer_enqueue(timer);
}

struct autodim * autodim_alloc(void)
{
	struct autodim *ad;

	ad = zalloc(sizeof(*ad));
	if (!ad)
		return NULL;

	return ad;
}

int autodim_init(struct autodim *ad, struct backlight *bl,
		 unsigned int max_percent, int dim_fully_dark)
{
	LIST_HEAD(dir_entries);
	struct dir_entry *dir_entry;
	int err, i, fd, count;
	char path[PATH_MAX + 1];

	ad->bl = bl;
	ad->bl->autodim_enabled++;

	count = list_directory(&dir_entries, "/dev/input");
	if (count <= 0) {
		logerr("Failed to list /dev/input\n");
		err = -ENOENT;
		goto error;
	}
	ad->fds = calloc(count, sizeof(*ad->fds));
	if (!ad->fds) {
		err = -ENOMEM;
		goto err_free_dir_entries;
	}
	i = 0;
	list_for_each_entry(dir_entry, &dir_entries, list) {
		if (dir_entry->type != DT_CHR)
			continue;

		snprintf(path, sizeof(path), "/dev/input/%s", dir_entry->name);
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			logerr("Failed to open %s\n", path);
			err = -ENOENT;
			goto err_free_fds;
		}

		err = fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);
		err |= fcntl(fd, F_SETSIG, SIGUSR1);
		err |= fcntl(fd, F_SETOWN, getpid());
		if (err) {
			logerr("Failed to set O_ASYNC\n");
			close(fd);
			err = -ENOENT;
			goto err_free_fds;
		}
		ad->fds[i++] = fd;
		logverbose("Autodim: Watching input device %s\n", path);
	}
	ad->nr_fds = i;
	dir_entries_free(&dir_entries);

	ad->max_percent = clamp(max_percent, 0, 100);
	ad->dim_fully_dark = dim_fully_dark;
	ad->bl_percent = max_percent;
	err = backlight_set_percentage(ad->bl, ad->bl_percent);
	if (err)
		goto err_free_fds;

	sleeptimer_init(&ad->timer, autodim_timer_callback);
	sleeptimer_set_timeout_relative(&ad->timer, AUTODIM_TIMER_INTERVAL);
	sleeptimer_enqueue(&ad->timer);

	logdebug("Auto-dimming enabled\n");

	return 0;

err_free_fds:
	free(ad->fds);
	ad->fds = NULL;
	ad->nr_fds = 0;
err_free_dir_entries:
	dir_entries_free(&dir_entries);
error:
	ad->bl->autodim_enabled--;

	return err;
}

void autodim_destroy(struct autodim *ad)
{
	unsigned int i;

	if (!ad)
		return;

	ad->bl->autodim_enabled--;

	sleeptimer_dequeue(&ad->timer);
	for (i = 0; i < ad->nr_fds; i++)
		close(ad->fds[i]);
	free(ad->fds);
	ad->fds = NULL;

	backlight_set_percentage(ad->bl, ad->max_percent);

	logdebug("Auto-dimming disabled\n");
}

void autodim_free(struct autodim *ad)
{
	if (ad)
		free(ad);
}

void autodim_handle_input_event(struct autodim *ad)
{
	logverbose("Autodim: Got input event.\n");
	if (ad->state) {
		ad->state = 0;
		autodim_handle_state(ad);
	}
}
