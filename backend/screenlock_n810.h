#ifndef BACKEND_SCREENLOCK_N810_H_
#define BACKEND_SCREENLOCK_N810_H_

#include "screenlock.h"
#include "fileaccess.h"

struct screenlock_n810 {
	struct screenlock screenlock;

	int switch_state;

	int kb_lock_evdev_fd;
	struct fileaccess *kb_lock_state_file;
	struct fileaccess *kb_disable_file;
	struct fileaccess *ts_disable_file;
};

struct screenlock * screenlock_n810_probe(void);

#endif /* BACKEND_SCREENLOCK_N810_H_ */
