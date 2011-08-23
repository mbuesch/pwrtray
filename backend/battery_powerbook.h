#ifndef BACKEND_BATTERY_POWERBOOK_H_
#define BACKEND_BATTERY_POWERBOOK_H_

#include "battery.h"

struct battery_powerbook {
	struct battery battery;

	struct fileaccess *info_file;
	struct fileaccess *stat_file;

	int on_ac;
	int max_charge;
	int charge;
};

#endif /* BACKEND_BATTERY_POWERBOOK_H_ */
