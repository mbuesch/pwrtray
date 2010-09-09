#ifndef BACKEND_AUTODIM_H_
#define BACKEND_AUTODIM_H_

#include "backlight.h"
#include "timer.h"


struct autodim {
	struct backlight *bl;
	int *fds;
	unsigned int nr_fds;
	struct sleeptimer timer;
};

struct autodim * autodim_alloc(void);
int autodim_init(struct autodim *ad, struct backlight *bl);

void autodim_destroy(struct autodim *ad);
void autodim_free(struct autodim *ad);

void autodim_handle_input_event(struct autodim *ad);

#endif /* BACKEND_AUTODIM_H_ */
