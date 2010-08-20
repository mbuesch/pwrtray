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
#include "backlight_omapfb.h"
#include "backlight_class.h"


Backlight::Backlight()
{
}

Backlight::~Backlight()
{
}

Backlight * Backlight::probe()
{
	Backlight *b;

	b = BacklightClass::probe();
	if (b)
		return b;

	b = BacklightOmapfb::probe();
	if (b)
		return b;

	return NULL;
}

int Backlight::minBrightness()
{
	return 0;
}

int Backlight::brightnessStep()
{
	return 1;
}

#include "backlight.moc"
