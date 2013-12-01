/*
 *   Copyright (C) 2013 Michael Buesch <m@bues.ch>
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

#include "probe.h"
#include "log.h"


void print_probe_message(const struct probe *probe)
{
	if (probe) {
		logdebug("Probing '%s' (%s @ %p)\n",
			 probe->name,
			 probe->func_name,
			 probe->func);
	}
}
