#ifndef BATTERY_N810_H_
#define BATTERY_N810_H_

#include "battery.h"


class BatteryN810 : public Battery
{
public:
	BatteryN810();
	virtual ~BatteryN810();

	static BatteryN810 * probe();

	virtual int maxCharge();
	virtual int currentCharge();
};

#endif /* BATTERY_N810_H_ */
