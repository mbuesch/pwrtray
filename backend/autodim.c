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

#include <string.h>


struct autodim * autodim_alloc(void)
{
	struct autodim *ad;

	ad = malloc(sizeof(*ad));
	if (!ad)
		return NULL;
	memset(ad, 0, sizeof(*ad));

	return ad;
}

int autodim_init(struct autodim *ad, struct backlight *bl)
{
	ad->bl = bl;

	return 0;
}

void autodim_destroy(struct autodim *ad)
{
	if (ad) {
	}
}

void autodim_free(struct autodim *ad)
{
	if (ad)
		free(ad);
}
