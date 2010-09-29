#ifndef BACKEND_SCREENLOCK_H_
#define BACKEND_SCREENLOCK_H_

#include "backlight.h"
#include "autodim.h"

struct screenlock {
	void (*destroy)(struct screenlock *s);
	void (*event)(struct screenlock *s);
};

struct screenlock * screenlock_probe();
void screenlock_destroy(struct screenlock *s);

#endif /* BACKEND_SCREENLOCK_H_ */
