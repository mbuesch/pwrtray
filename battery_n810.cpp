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

#include <iostream>

using namespace std;


BatteryN810::BatteryN810(SysFsFile *chgFile)
 : Battery (10000)
 , chargeFile (chgFile)
{
	update();
	timer->start();
}

BatteryN810::~BatteryN810()
{
	delete chargeFile;
}

BatteryN810 * BatteryN810::probe()
{
	SysFsFile *charge = SysFsFile::openFile("/devices/platform/n810bm/batt_charge");
	if (!charge)
		return NULL;
	return new BatteryN810(charge);
}

void BatteryN810::update()
{
	int value;
	bool valueChanged = false;

	if (!chargeFile->readInt(&value)) {
		cerr << "WARNING: Failed to read battery charge file" << endl;
		return;
	}
	if (value != curChg)
		valueChanged = true;
	curChg = value;

	if (valueChanged)
		emit stateChanged();
}

int BatteryN810::maxCharge()
{
	return 100;
}

int BatteryN810::currentCharge()
{
	return curChg;
}

#include "battery_n810.moc"
