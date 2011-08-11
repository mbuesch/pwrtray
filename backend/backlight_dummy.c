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

#include "backlight_dummy.h"
#include "util.h"


static int backlight_dummy_max_brightness(struct backlight *b)
{
	return 0;
}

static void backlight_dummy_destroy(struct backlight *b)
{
	struct backlight_dummy *bd = container_of(b, struct backlight_dummy, backlight);

	free(bd);
}

struct backlight * backlight_dummy_probe(void)
{
	struct backlight_dummy *bd;

	bd = zalloc(sizeof(*bd));
	if (!bd)
		return NULL;

	backlight_init(&bd->backlight, "dummy");
	bd->backlight.max_brightness = backlight_dummy_max_brightness;
	bd->backlight.destroy = backlight_dummy_destroy;

	return &bd->backlight;
}
