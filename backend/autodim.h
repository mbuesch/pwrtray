#ifndef BACKEND_AUTODIM_H_
#define BACKEND_AUTODIM_H_

#include "backlight.h"


struct autodim {
	struct backlight *bl;
};

struct autodim * autodim_alloc(void);
int autodim_init(struct autodim *ad, struct backlight *bl);

void autodim_destroy(struct autodim *ad);
void autodim_free(struct autodim *ad);

#endif /* BACKEND_AUTODIM_H_ */
