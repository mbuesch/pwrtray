#ifndef BACKEND_SCREENLOCK_H_
#define BACKEND_SCREENLOCK_H_

#include "backlight.h"
#include "autodim.h"


struct screenlock {
	void (*destroy)(struct screenlock *s);
	void (*event)(struct screenlock *s);

	/* Internal */
	struct backlight *backlight;
};

struct screenlock * screenlock_probe(struct backlight *bl);
void screenlock_destroy(struct screenlock *s);

#endif /* BACKEND_SCREENLOCK_H_ */
