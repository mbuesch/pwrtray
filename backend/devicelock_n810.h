#ifndef BACKEND_DEVICELOCK_N810_H_
#define BACKEND_DEVICELOCK_N810_H_

#include "devicelock.h"
#include "fileaccess.h"

struct devicelock_n810 {
	struct devicelock devicelock;

	int switch_state;

	int kb_lock_evdev_fd;
	struct fileaccess *kb_lock_state_file;
	struct fileaccess *kb_disable_file;
	struct fileaccess *ts_disable_file;
};

#endif /* BACKEND_DEVICELOCK_N810_H_ */
