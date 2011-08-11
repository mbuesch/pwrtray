#ifndef BACKEND_BATTERY_H_
#define BACKEND_BATTERY_H_

#include "timer.h"
#include "api.h"


struct battery {
	const char *name;

	int (*on_ac)(struct battery *b);
	int (*min_charge)(struct battery *b);
	int (*max_charge)(struct battery *b);
	int (*current_charge)(struct battery *b);

	void (*destroy)(struct battery *b);
	int (*update)(struct battery *b);
	unsigned int poll_interval;

	/* Internal */
	struct sleeptimer timer;
};

void battery_init(struct battery *b, const char *name);

struct battery * battery_probe(void);
void battery_destroy(struct battery *b);

int battery_fill_pt_message_stat(struct battery *b, struct pt_message *msg);
int battery_notify_state_change(struct battery *b);

#endif /* BACKEND_BATTERY_H_ */
