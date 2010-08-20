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

#include "backlight_omapfb.h"

#include <iostream>

#define BASEPATH	"/devices/platform/omapfb/panel"

using namespace std;


BacklightOmapfb::BacklightOmapfb(int maxLvl, SysFsFile *level)
 : maxLevel (maxLvl)
 , curLevel (~0)
 , levelFile (level)
{
	update();
}

BacklightOmapfb::~BacklightOmapfb()
{
	delete levelFile;
}

BacklightOmapfb * BacklightOmapfb::probe()
{
	int maxLevel;
	SysFsFile *file, *levelFile;

	file = SysFsFile::openFile(QString(BASEPATH "/") + "backlight_max");
	if (!file)
		return NULL;
	if (!file->readInt(&maxLevel)) {
		delete file;
		return NULL;
	}
	delete file;
	levelFile = SysFsFile::openFile(QString(BASEPATH "/") + "backlight_level",
					QFile::ReadWrite);
	if (!file)
		return NULL;

	return new BacklightOmapfb(maxLevel, levelFile);
}

void BacklightOmapfb::update()
{
	bool valueChanged = false;
	int newLevel;

	if (!levelFile->readInt(&newLevel)) {
		cerr << "WARNING: Failed to read backlight level file" << endl;
		return;
	}
	if (newLevel != curLevel)
		valueChanged = true;
	curLevel = newLevel;

	if (valueChanged)
		emit stateChanged();
}

int BacklightOmapfb::maxBrightness()
{
	return maxLevel;
}

int BacklightOmapfb::currentBrightness()
{
	return curLevel;
}

void BacklightOmapfb::setBrightness(int val)
{
	val = min(maxBrightness(), val);
	val = max(minBrightness(), val);
	if (!levelFile->writeInt(val)) {
		cerr << "WARNING: Failed to set backlight brightness" << endl;
		return;
	}
	curLevel = val;
	update();
}

#include "backlight_omapfb.moc"
