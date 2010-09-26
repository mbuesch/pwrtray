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
#include <assert.h>


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
	struct autodim_step *step;

	assert(ad->state < ARRAY_SIZE(ad->steps));
	step = &ad->steps[ad->state];

	if (ad->idle_seconds >= step->second) {
		autodim_set_backlight(ad, step->percent);
		if (ad->state < ARRAY_SIZE(ad->steps) - 1)
			ad->state++;
	}
}

static void autodim_timer_callback(struct sleeptimer *timer)
{
	struct autodim *ad = container_of(timer, struct autodim, timer);

	if (ad->idle_seconds < 0xFFFFFFFF) {
		ad->idle_seconds++;
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

static int autodim_steps_get(struct autodim *ad, struct config_file *config)
{
	char *string, *s, *next;
	unsigned int stepnr = 0;
	int ret = 0;

	string = strdup(config_get(config, "BACKLIGHT", "autodim_steps",
				   "15/75 25/50 50/25 75/10"));
	if (!string)
		return -ENOMEM;
	s = string_strip(string);
	while (1) {
		next = strchr(s, ' ');
		if (next) {
			next[0] = '\0';
			next++;
		}
		if (strlen(s) == 0)
			break;
		if (stepnr >= ARRAY_SIZE(ad->steps)) {
			logerr("Too many autodim_steps set in config file (max %d)\n",
			       (int)ARRAY_SIZE(ad->steps));
			ret = -EINVAL;
			goto out;
		}
		if (sscanf(s, "%u/%u", &ad->steps[stepnr].second,
			   &ad->steps[stepnr].percent) != 2) {
			logerr("Failed to parse autodim_steps config value\n");
			ret = -EINVAL;
			goto out;
		}
		if (stepnr > 0) {
			if (ad->steps[stepnr - 1].second >= ad->steps[stepnr].second) {
				logerr("Error: autodim_steps, previous step idle-seconds "
				       "value bigger than adjacent value\n");
				ret = -EINVAL;
				goto out;
			}
		}
		stepnr++;
		if (!next)
			break;
		s = string_strip(next);
	}
	ad->nr_steps = stepnr;
out:
	free(string);

	return ret;
}

int autodim_init(struct autodim *ad, struct backlight *bl,
		 struct config_file *config)
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

	ad->max_percent = clamp(config_get_int(config,
					       "BACKLIGHT",
					       "autodim_max",
					       100),
				0, 100);
	ad->bl_percent = ad->max_percent;
	err = autodim_steps_get(ad, config);
	if (err)
		goto err_free_fds;
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

void autodim_suspend(struct autodim *ad)
{
	if (!ad)
		return;
	if (!ad->suspended) {
		//TODO stop the timer. Stop events.
		logdebug("Auto-dimming suspended\n");
	}
	ad->suspended++;
}

void autodim_resume(struct autodim *ad)
{
	if (!ad)
		return;
	ad->suspended--;
	if (!ad->suspended) {
		ad->idle_seconds = 0;
		autodim_handle_state(ad);
		logdebug("Auto-dimming resumed\n");
	}
}

void autodim_handle_input_event(struct autodim *ad)
{
	logverbose("Autodim: Got input event.\n");
	ad->idle_seconds = 0;
	ad->state = 0;
	autodim_set_backlight(ad, ad->max_percent);
}
