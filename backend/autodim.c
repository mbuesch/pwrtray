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

#include "autodim.h"
#include "log.h"
#include "fileaccess.h"
#include "util.h"
#include "main.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <math.h>


static void autodim_set_backlight(struct autodim *ad, unsigned int percent)
{
	struct battery *battery = backend.battery;
	struct backlight *backlight = backend.backlight;

	if (battery && battery->on_ac(battery)) {
		if (!backlight->autodim_enabled_on_ac)
			percent = ad->max_percent;
	}

	percent = min(percent, ad->max_percent);

	if (percent != ad->bl_percent) {
		ad->bl_percent = percent;
		backlight_set_percentage(ad->bl, percent);
		logverbose("Autodim: Set backlight to %u percent.\n", percent);
	}
}

static void autodim_handle_state(struct autodim *ad)
{
	struct autodim_step *step;

	if (ad->state < ad->nr_steps) {
		step = &ad->steps[ad->state];
		ad->state++;

		autodim_set_backlight(ad, step->percent);
	}
}

static void autodim_timer_stop(struct autodim *ad)
{
	sleeptimer_dequeue(&ad->timer);
}

static void autodim_timer_start(struct autodim *ad)
{
	unsigned int sec;

	if (ad->state == 0)
		sec = ad->steps[0].second;
	else if (ad->state < ad->nr_steps)
		sec = ad->steps[ad->state].second - ad->steps[ad->state - 1].second;
	else
		return;

	logverbose("autodim: Next event in %u sec\n", sec);
	sleeptimer_set_timeout_relative(&ad->timer, sec * 1000);
	sleeptimer_enqueue(&ad->timer);
}

static void autodim_timer_callback(struct sleeptimer *timer)
{
	struct autodim *ad = container_of(timer, struct autodim, timer);

	logverbose("autodim: timer triggered\n");
	autodim_handle_state(ad);
	autodim_timer_start(ad);
}

struct autodim * autodim_alloc(void)
{
	struct autodim *ad;

	ad = zalloc(sizeof(*ad));
	if (!ad)
		return NULL;

	return ad;
}

static int autodim_force_realloc_steps(struct autodim *ad, unsigned int newcount)
{
	void *buf;

	buf = realloc(ad->steps, newcount * sizeof(*ad->steps));
	if (!buf) {
		logerr("Out of memory\n");
		return -ENOMEM;
	}
	ad->steps = buf;
	ad->nr_allocated_steps = newcount;

	return 0;
}

static int autodim_realloc_steps(struct autodim *ad, unsigned int newcount)
{
	if (newcount > ad->nr_allocated_steps)
		return autodim_force_realloc_steps(ad, newcount);
	return 0;
}

static int autodim_smoothen_steps(struct autodim *ad)
{
	struct autodim_step *old_steps, *cur, *next;
	unsigned int old_i, new_i, nr_old_steps, sec;
	float secdiff, perdiff, ppsec, perc;
	int ret = 0;

	old_steps = calloc(ad->nr_steps, sizeof(*old_steps));
	if (!old_steps)
		return -ENOMEM;
	memcpy(old_steps, ad->steps, ad->nr_steps * sizeof(*old_steps));
	nr_old_steps = ad->nr_steps;

	ad->nr_steps = 0;
	for (old_i = 0, new_i = 0; ; old_i++) {
		ret = autodim_realloc_steps(ad, new_i + 1);
		if (ret)
			goto out;
		cur = &old_steps[old_i];
		ad->steps[new_i++] = *cur;
		if (old_i + 1 >= nr_old_steps)
			break;
		if (cur->percent == 0)
			break;
		next = &old_steps[old_i + 1];
		if (next->percent == cur->percent - 1)
			continue;

		/* Make intermediate steps */
		secdiff = (float)next->second - (float)cur->second;
		perdiff = (float)cur->percent - (float)next->percent;
		if (secdiff <= 0 || perdiff <= 0)
			continue;
		ppsec = perdiff / secdiff;
		sec = cur->second + 1;
		perc = (float)cur->percent - ppsec;
		while (sec < next->second) {
			ret = autodim_realloc_steps(ad, new_i + 1);
			if (ret)
				goto out;
			ad->steps[new_i].second = sec;
			ad->steps[new_i].percent = max((int)roundf(perc), 0);
			new_i++;
			perc -= ppsec;
			sec += 1;
		}
	}
	ad->nr_steps = new_i;

out:
	free(old_steps);

	return ret;
}

static int autodim_optimize_steps(struct autodim *ad)
{
	struct autodim_step *cur, *next;
	unsigned int i;
	int err;

	/* Remove duplicates */
	for (i = 0; ad->nr_steps && i < ad->nr_steps - 1; i++) {
		cur = &ad->steps[i];
		next = &ad->steps[i + 1];
		if (cur->percent == next->percent ||
		    cur->second >= next->second) {
			/* Remove next. */
			memmove(&ad->steps[i + 1], &ad->steps[i + 2],
				(ad->nr_steps - i - 2) * sizeof(*cur));
			ad->nr_steps--;
		}
	}

	err = autodim_force_realloc_steps(ad, ad->nr_steps);
	if (err)
		return err;

	return 0;
}

static void autodim_dump_steps(struct autodim *ad, const char *info)
{
	unsigned int i;

	loginfo("Autodim steps (%s): ", info);
	for (i = 0; i < ad->nr_steps; i++) {
		loginfo("%u/%u%s", ad->steps[i].second, ad->steps[i].percent,
			(i + 1 == ad->nr_steps) ? "" : " ");
	}
	loginfo("\n");
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
		if (strempty(s))
			break;
		ret = autodim_realloc_steps(ad, stepnr + 1);
		if (ret)
			goto out;
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

	if (loglevel_is_debug())
		autodim_dump_steps(ad, "parsed-config");
	if (config_get_bool(config, "BACKLIGHT", "autodim_smooth", 0)) {
		ret = autodim_smoothen_steps(ad);
		if (ret)
			goto out;
		if (loglevel_is_debug())
			autodim_dump_steps(ad, "smoothened");
	}
	ret = autodim_optimize_steps(ad);
	if (ret)
		goto out;
	if (loglevel_is_debug())
		autodim_dump_steps(ad, "optimized");

out:
	free(string);

	return ret;
}

int autodim_init(struct autodim *ad, struct backlight *bl,
		 struct config_file *config)
{
	LIST_HEAD(dir_entries);
	struct dir_entry *dir_entry;
	int err, i, fd, count, percent;
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
			if (errno == ENODEV)
				continue;
			logerr("Failed to open %s: %s\n",
			       path, strerror(errno));
			continue; /* Continue anyway */
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

	percent = backlight_get_percentage(ad->bl);
	if (percent < 0)
		percent = 100;
	ad->bl_percent = percent;
	ad->max_percent = percent;
	err = autodim_steps_get(ad, config);
	if (err)
		goto err_free_fds;

	sleeptimer_init(&ad->timer, "autodim", autodim_timer_callback);
	autodim_timer_start(ad);

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

	autodim_timer_stop(ad);
	for (i = 0; i < ad->nr_fds; i++)
		close(ad->fds[i]);
	free(ad->fds);
	ad->fds = NULL;

	logdebug("Auto-dimming disabled\n");
}

void autodim_free(struct autodim *ad)
{
	if (ad) {
		free(ad->steps);
		memset(ad, 0, sizeof(*ad));
		free(ad);
	}
}

void autodim_suspend(struct autodim *ad)
{
	if (!ad)
		return;
	if (!ad->suspended) {
		autodim_timer_stop(ad);
		//TODO disable input events.
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
		autodim_handle_input_event(ad);
		autodim_timer_start(ad);
		logdebug("Auto-dimming resumed\n");
	}
}

void autodim_set_max_percent(struct autodim *ad, int max_percent)
{
	if (max_percent < 0)
		max_percent = 100;
	max_percent = clamp(max_percent, 0, 100);
	if (ad->max_percent != (unsigned int)max_percent) {
		ad->max_percent = (unsigned int)max_percent;
		autodim_handle_input_event(ad);
	}
}

void autodim_handle_input_event(struct autodim *ad)
{
	logverbose("Autodim: Got input event.\n");
	ad->state = 0;
	autodim_timer_start(ad);
	autodim_set_backlight(ad, ad->max_percent);
}

void autodim_handle_battery_event(struct autodim *ad)
{
	logverbose("Autodim: Got battery event.\n");
	/* Handle possible change of "on-AC" state. */
	autodim_set_backlight(ad, ad->bl_percent);
}
