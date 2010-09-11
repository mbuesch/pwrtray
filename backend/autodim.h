#ifndef BACKEND_AUTODIM_H_
#define BACKEND_AUTODIM_H_

#include "backlight.h"
#include "timer.h"
#include "conf.h"


struct autodim_step {
	unsigned int second;
	unsigned int percent;
};

struct autodim {
	struct backlight *bl;
	int *fds;
	unsigned int nr_fds;
	struct sleeptimer timer;

	unsigned int idle_seconds;
	unsigned int state;
	unsigned int bl_percent;
	unsigned int max_percent;
	struct autodim_step steps[10];
	unsigned int nr_steps;
};

struct autodim * autodim_alloc(void);
int autodim_init(struct autodim *ad, struct backlight *bl,
		 struct config_file *config);

void autodim_destroy(struct autodim *ad);
void autodim_free(struct autodim *ad);

void autodim_handle_input_event(struct autodim *ad);

#endif /* BACKEND_AUTODIM_H_ */
