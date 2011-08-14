#ifndef BACKEND_BATTERY_ACPI_H_
#define BACKEND_BATTERY_ACPI_H_

#include "battery.h"

#include <limits.h>


struct battery_acpi {
	struct battery battery;

	int on_ac;
	int charge_max;
	int charge_now;

	char ac_online_filename[NAME_MAX + 1];
	char charge_max_filename[NAME_MAX + 1];
	char charge_now_filename[NAME_MAX + 1];
};

struct battery * battery_acpi_probe(void);

#endif /* BACKEND_BATTERY_ACPI_H_ */