#ifndef BACKLIGHT_H_
#define BACKLIGHT_H_

#include <QtCore/QObject>


class Backlight : public QObject
{
	Q_OBJECT
public:
	Backlight();
	virtual ~Backlight();

	static Backlight * probe();

	virtual int minBrightness();
	virtual int maxBrightness() = 0;
	virtual int brightnessStep();
	virtual int currentBrightness() = 0;
	virtual void setBrightness(int val) = 0;

signals:
	void stateChanged();
};

#endif /* BACKLIGHT_H_ */
