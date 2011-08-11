#ifndef BACKEND_BATTERY_DUMMY_H_
#define BACKEND_BATTERY_DUMMY_H_

#include "battery.h"

struct battery_dummy {
	struct battery battery;
};

struct battery * battery_dummy_probe(void);

#endif /* BACKEND_BATTERY_DUMMY_H_ */
