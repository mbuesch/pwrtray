#ifndef BACKLIGHT_OMAPFB_H_
#define BACKLIGHT_OMAPFB_H_

#include "backlight.h"
#include "fileaccess.h"


class BacklightOmapfb : public Backlight
{
	Q_OBJECT
public:
	BacklightOmapfb(int maxLvl, SysFsFile *level);
	virtual ~BacklightOmapfb();

	static BacklightOmapfb * probe();

	virtual int maxBrightness();
	virtual int currentBrightness();
	virtual void setBrightness(int val);

protected:
	void update();

protected:
	int maxLevel;
	int curLevel;
	SysFsFile *levelFile;
};

#endif /* BACKLIGHT_OMAPFB_H_ */
