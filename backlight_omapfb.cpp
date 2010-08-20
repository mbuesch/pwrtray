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
