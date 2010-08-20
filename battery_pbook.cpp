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

#include "battery_pbook.h"

#include <iostream>

using namespace std;


BatteryPbook::BatteryPbook(ProcFsFile *info, ProcFsFile *stat)
 : Battery(1000)
 , infoFile (info)
 , statFile (stat)
 , ac (false)
 , maxChg (~0)
 , curChg (~0)
{
	update();
	timer->start();
}

BatteryPbook::~BatteryPbook()
{
	delete infoFile;
	delete statFile;
}

BatteryPbook * BatteryPbook::probe()
{
	ProcFsFile *info = ProcFsFile::openFile("/pmu/info");
	ProcFsFile *stat = ProcFsFile::openFile("/pmu/battery_0");
	if (!info || !stat) {
		delete info;
		delete stat;
		return NULL;
	}
	return new BatteryPbook(info, stat);
}

void BatteryPbook::update()
{
	bool ok;
	bool valueChanged = false;
	bool gotAC = false, gotMaxChg = false, gotCurChg = false;
	unsigned int value;
	QStringList lines;
	QStringList::iterator i;
	if (!infoFile->readTextLines(&lines)) {
		cerr << "WARNING: Failed to read battery info file" << endl;
		return;
	}
	for (i = lines.begin(); i != lines.end(); ++i) {
		QStringList elements((*i).split(':'));
		if (elements.size() != 2)
			continue;
		QString elementId(elements[0].trimmed());
		if (elementId == "AC Power") {
			value = elements[1].toUInt(&ok);
			if (ok) {
				if (ac != value)
					valueChanged = true;
				ac = value;
				gotAC = true;
			}
		}
		if (gotAC)
			break;
	}
	if (!statFile->readTextLines(&lines)) {
		cerr << "WARNING: Failed to read battery status file" << endl;
		return;
	}
	for (i = lines.begin(); i != lines.end(); ++i) {
		QStringList elements((*i).split(':'));
		if (elements.size() != 2)
			continue;
		QString elementId(elements[0].trimmed());
		if (elementId == "max_charge") {
			value = elements[1].toUInt(&ok);
			if (ok) {
				if (maxChg != value)
					valueChanged = true;
				maxChg = value;
				gotMaxChg = true;
			}
		}
		if (elementId == "charge") {
			value = elements[1].toUInt(&ok);
			if (ok) {
				if (curChg != value)
					valueChanged = true;
				curChg = value;
				gotCurChg = true;
			}
		}
		if (gotMaxChg && gotCurChg)
			break;
	}
	if (!gotAC)
		cerr << "WARNING: Failed to get AC status" << endl;
	if (!gotMaxChg)
		cerr << "WARNING: Failed to get max battery charge value" << endl;
	if (!gotCurChg)
		cerr << "WARNING: Failed to get current battery charge value" << endl;
	if (valueChanged)
		emit stateChanged();
}

bool BatteryPbook::onAC()
{
	return ac;
}

int BatteryPbook::maxCharge()
{
	return maxChg;
}

int BatteryPbook::currentCharge()
{
	return curChg;
}

#include "battery_pbook.moc"
