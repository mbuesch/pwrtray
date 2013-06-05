#ifndef BACKEND_BATTERY_H_
#define BACKEND_BATTERY_H_

#include "timer.h"
#include "api.h"
#include "probe.h"


struct battery {
	const char *name;

	/* Returns 1 if on AC; 0 if not. */
	int (*on_ac)(struct battery *b);
	/* Enable/disable charging. Returns 0 on success or negative error code. */
	int (*charger_enable)(struct battery *b, int enable);
	/* Returns 1 if charging the battery; 0 if not */
	int (*charging)(struct battery *b);
	/* Returns the minimum battery level constant value */
	int (*min_level)(struct battery *b);
	/* Returns the maximum battery level constant value */
	int (*max_level)(struct battery *b);
	/* Get the battery charge level value */
	int (*charge_level)(struct battery *b);
	/* Returns the battery capacity in mAh. 0 if no battery present.
	 * Negative if unknown */
	int (*capacity_mAh)(struct battery *b);
	/* Returns the measured battery current in mA.
	 * This is charge current if charging and drain current if draining.
	 * Negative value is returned on error */
	int (*current_mA)(struct battery *b);
	/* Returns the battery temperature in K. Negative is not supported. */
	int (*temperature_K)(struct battery *b);

	void (*destroy)(struct battery *b);
	int (*update)(struct battery *b);
	unsigned int poll_interval;

	/* Internal */
	struct sleeptimer timer;
	int emergency_handled;
};

void battery_init(struct battery *b, const char *name);

struct battery * battery_probe(void);
void battery_destroy(struct battery *b);

int battery_fill_pt_message_stat(struct battery *b, struct pt_message *msg);
int battery_notify_state_change(struct battery *b);

DECLARE_PROBES(battery);
#define BATTERY_PROBE(_name, _func)	DEFINE_PROBE(battery, _name, _func)

#endif /* BACKEND_BATTERY_H_ */
