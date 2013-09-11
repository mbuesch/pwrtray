#ifndef BACKEND_BATTERY_ACPI_H_
#define BACKEND_BATTERY_ACPI_H_

#include "battery.h"


struct battery_acpi {
	struct battery battery;

	int on_ac;
	int charge_max;
	int charge_now;

	char *ac_online_filename;
	char *charge_max_filename;
	char *charge_now_filename;
};

#endif /* BACKEND_BATTERY_ACPI_H_ */
