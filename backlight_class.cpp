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

#include "backlight_class.h"
#include "fileaccess.h"

#include <iostream>

#define BASEPATH	"/class/backlight"

using namespace std;


BacklightClass::BacklightClass(int _maxBrightness,
			       SysFsFile *actualBr,
			       SysFsFile *setBr)
 : maxBr (_maxBrightness)
 , actualBrFile (actualBr)
 , setBrFile (setBr)
{
	update();
}

BacklightClass::~BacklightClass()
{
	delete actualBrFile;
	delete setBrFile;
}

void BacklightClass::update()
{
	bool valueChanged = false;
	int newBrightness;

	if (!actualBrFile->readInt(&newBrightness)) {
		cerr << "WARNING: Failed to read actual backlight brightness" << endl;
		return;
	}
	if (newBrightness != curBr)
		valueChanged = true;
	curBr = newBrightness;

	if (valueChanged)
		emit stateChanged();
}

BacklightClass * BacklightClass::probe()
{
	QStringList list;
	SysFsFile *file, *actualBr, *setBr;
	int maxBr;

	if (!SysFsFile::listDir(BASEPATH, &list,
				QDir::Dirs | QDir::NoDotAndDotDot))
		return NULL;
	if (list.size() < 1)
		return NULL;
	QString path(QString(BASEPATH "/") + list[0]);
	file = SysFsFile::openFile(path + "/max_brightness");
	if (!file)
		return NULL;
	if (!file->readInt(&maxBr)) {
		delete file;
		return NULL;
	}
	delete file;
	actualBr = SysFsFile::openFile(path + "/actual_brightness");
	if (!actualBr)
		return NULL;
	setBr = SysFsFile::openFile(path + "/brightness", QFile::ReadWrite);
	if (!setBr) {
		cerr << "WARNING: Failed to open backlight brightness adjusting file. "
			"Do we have sufficient permission?" << endl;
		delete actualBr;
		return NULL;
	}

	return new BacklightClass(maxBr, actualBr, setBr);
}

int BacklightClass::maxBrightness()
{
	return maxBr;
}

int BacklightClass::currentBrightness()
{
	return curBr;
}

void BacklightClass::setBrightness(int val)
{
	val = min(maxBrightness(), val);
	val = max(minBrightness(), val);
	if (!setBrFile->writeInt(val)) {
		cerr << "WARNING: Failed to set backlight brightness" << endl;
		return;
	}
	curBr = val;
	update();
}

#include "backlight_class.moc"