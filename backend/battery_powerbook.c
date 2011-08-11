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

#include "battery_powerbook.h"
#include "fileaccess.h"
#include "log.h"
#include "util.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>


static int battery_powerbook_update(struct battery *b)
{
	struct battery_powerbook *bp = container_of(b, struct battery_powerbook, battery);
	LIST_HEAD(lines);
	struct text_line *tl;
	int err;
	unsigned int value;
	int got_ac = 0, got_max_chg = 0, got_cur_chg = 0;
	int value_changed = 0;

	err = file_read_text_lines(bp->info_file, &lines, 0);
	if (err || list_empty(&lines)) {
		logerr("WARNING: Failed to read battery info file\n");
		return -ETXTBSY;
	}
	list_for_each_entry(tl, &lines, list) {
		char *id, *elem;

		elem = strchr(tl->text, ':');
		if (!elem)
			continue;
		elem[0] = '\0';
		elem++;
		elem = string_strip(elem);
		id = string_strip(tl->text);

		if (strcmp(id, "AC Power") == 0) {
			if (sscanf(elem, "%u", &value) == 1) {
				if (value != bp->on_ac)
					value_changed = 1;
				bp->on_ac = value;
				got_ac = 1;
			}
		}

		if (got_ac)
			break;
	}
	text_lines_free(&lines);

	err = file_read_text_lines(bp->stat_file, &lines, 0);
	if (err || list_empty(&lines)) {
		logerr("WARNING: Failed to read battery stat file\n");
		return -ETXTBSY;
	}
	list_for_each_entry(tl, &lines, list) {
		char *id, *elem;

		elem = strchr(tl->text, ':');
		if (!elem)
			continue;
		elem[0] = '\0';
		elem++;
		elem = string_strip(elem);
		id = string_strip(tl->text);

		if (strcmp(id, "max_charge") == 0) {
			if (sscanf(elem, "%u", &value) == 1) {
				if (value != bp->max_charge)
					value_changed = 1;
				bp->max_charge = value;
				got_max_chg = 1;
			}
		}
		if (strcmp(id, "charge") == 0) {
			if (sscanf(elem, "%u", &value) == 1) {
				if (value != bp->charge)
					value_changed = 1;
				bp->charge = value;
				got_cur_chg = 1;
			}
		}

		if (got_max_chg && got_cur_chg)
			break;
	}
	text_lines_free(&lines);

	if (!got_ac)
		logerr("WARNING: Failed to get AC status\n");
	if (!got_max_chg)
		logerr("WARNING: Failed to get max battery charge value\n");
	if (!got_cur_chg)
		logerr("WARNING: Failed to get current battery charge value\n");
	if (value_changed)
		battery_notify_state_change(b);

	return 0;
}

static int battery_powerbook_on_ac(struct battery *b)
{
	struct battery_powerbook *bp = container_of(b, struct battery_powerbook, battery);

	return bp->on_ac;
}

static int battery_powerbook_max_charge(struct battery *b)
{
	struct battery_powerbook *bp = container_of(b, struct battery_powerbook, battery);

	return bp->max_charge;
}

static int battery_powerbook_current_charge(struct battery *b)
{
	struct battery_powerbook *bp = container_of(b, struct battery_powerbook, battery);

	return bp->charge;
}

static void battery_powerbook_destroy(struct battery *b)
{
	struct battery_powerbook *bp = container_of(b, struct battery_powerbook, battery);

	file_close(bp->info_file);
	file_close(bp->stat_file);

	free(bp);
}

struct battery * battery_powerbook_probe(void)
{
	struct fileaccess *info, *stat;
	struct battery_powerbook *bp;

	info = procfs_file_open(O_RDONLY, "/pmu/info");
	stat = procfs_file_open(O_RDONLY, "/pmu/battery_0");
	if (!info || !stat)
		goto err_close;

	bp = zalloc(sizeof(*bp));
	if (!bp)
		goto err_close;

	bp->info_file = info;
	bp->stat_file = stat;

	battery_init(&bp->battery, "powerbook");
	bp->battery.destroy = battery_powerbook_destroy;
	bp->battery.update = battery_powerbook_update;
	bp->battery.on_ac = battery_powerbook_on_ac;
	bp->battery.max_charge = battery_powerbook_max_charge;
	bp->battery.current_charge = battery_powerbook_current_charge;
	bp->battery.poll_interval = 1000;

	return &bp->battery;

err_close:
	file_close(info);
	file_close(stat);
	return NULL;
}
