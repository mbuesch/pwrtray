#include "battery_n810.h"


BatteryN810::BatteryN810()
{
}

BatteryN810::~BatteryN810()
{
}

BatteryN810 * BatteryN810::probe()
{
	//TODO
	return new BatteryN810;
}

int BatteryN810::maxCharge()
{
	//TODO
	return 0;
}

int BatteryN810::currentCharge()
{
	//TODO
	return 0;
}
