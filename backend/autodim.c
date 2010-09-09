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
#include <signal.h>
#include <errno.h>


#define AUTODIM_TIMER_INTERVAL	1000


static void autodim_timer_callback(struct sleeptimer *timer)
{
	struct autodim *ad = container_of(timer, struct autodim, timer);

printf("AUTODIM TIMER\n");
	//TODO
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

int autodim_init(struct autodim *ad, struct backlight *bl)
{
	LIST_HEAD(dir_entries);
	struct dir_entry *dir_entry;
	int err, i, fd, count;
	char path[PATH_MAX + 1];

	ad->bl = bl;

	count = list_directory(&dir_entries, "/dev/input");
	if (count <= 0) {
		logerr("Failed to list /dev/input\n");
		return -ENOENT;
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
	}
	ad->nr_fds = i;
	dir_entries_free(&dir_entries);

	sleeptimer_init(&ad->timer, autodim_timer_callback);
	sleeptimer_set_timeout_relative(&ad->timer, AUTODIM_TIMER_INTERVAL);
	sleeptimer_enqueue(&ad->timer);

	logdebug("Auto-dimming enabled\n");

	return 0;

err_free_fds:
	free(ad->fds);
	ad->fds = NULL;
err_free_dir_entries:
	dir_entries_free(&dir_entries);

	return err;
}

void autodim_destroy(struct autodim *ad)
{
	unsigned int i;

	if (!ad)
		return;

	sleeptimer_dequeue(&ad->timer);
	for (i = 0; i < ad->nr_fds; i++)
		close(ad->fds[i]);
	free(ad->fds);
	ad->fds = NULL;

	logdebug("Auto-dimming disabled\n");
}

void autodim_free(struct autodim *ad)
{
	if (ad)
		free(ad);
}

void autodim_handle_input_event(struct autodim *ad)
{
printf("INPUT EV\n");
}
