#ifndef BATTERY_N810_H_
#define BATTERY_N810_H_

#include "battery.h"
#include "fileaccess.h"


class BatteryN810 : public Battery
{
	Q_OBJECT
public:
	BatteryN810(SysFsFile *chgFile);
	virtual ~BatteryN810();

	static BatteryN810 * probe();

	virtual int maxCharge();
	virtual int currentCharge();

protected:
	void update();

protected:
	SysFsFile *chargeFile;
	int curChg;
};

#endif /* BATTERY_N810_H_ */
