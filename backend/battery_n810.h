#ifndef BACKEND_BATTERY_N810_H_
#define BACKEND_BATTERY_N810_H_

#include "battery.h"

struct battery_n810 {
	struct battery battery;

	struct fileaccess *level_file;
	int charge;
};

#endif /* BACKEND_BATTERY_N810_H_ */
