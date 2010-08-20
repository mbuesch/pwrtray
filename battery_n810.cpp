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

#include "battery_n810.h"


BatteryN810::BatteryN810()
{
}

BatteryN810::~BatteryN810()
{
}

BatteryN810 * BatteryN810::probe()
{
	//TODO
	return new BatteryN810;
}

int BatteryN810::maxCharge()
{
	//TODO
	return 0;
}

int BatteryN810::currentCharge()
{
	//TODO
	return 0;
}
