#ifndef BACKLIGHT_CLASS_H_
#define BACKLIGHT_CLASS_H_

#include "backlight.h"
#include "fileaccess.h"


class BacklightClass : public Backlight
{
	Q_OBJECT
public:
	BacklightClass(int _maxBrightness,
		       SysFsFile *actualBr,
		       SysFsFile *setBr);
	virtual ~BacklightClass();

	static BacklightClass * probe();

	virtual int maxBrightness();
	virtual int currentBrightness();
	virtual void setBrightness(int val);

protected:
	void update();

protected:
	int maxBr;
	int curBr;
	SysFsFile *actualBrFile;
	SysFsFile *setBrFile;
};

#endif /* BACKLIGHT_CLASS_H_ */
