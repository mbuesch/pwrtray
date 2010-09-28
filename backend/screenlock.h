#ifndef BACKEND_SCREENLOCK_H_
#define BACKEND_SCREENLOCK_H_

#include "backlight.h"
#include "autodim.h"

#include <unistd.h>
#include <sys/types.h>


struct screenlock {
	void (*destroy)(struct screenlock *s);
	void (*event)(struct screenlock *s);

	/* Internal */
	struct backlight *backlight;
	pid_t x11lock_helper;
};

struct screenlock * screenlock_probe(struct backlight *bl);
void screenlock_destroy(struct screenlock *s);

int block_x11_input(struct screenlock *s);
void unblock_x11_input(struct screenlock *s);

#endif /* BACKEND_SCREENLOCK_H_ */
