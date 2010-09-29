#ifndef BACKEND_X11LOCK_H_ 
#define BACKEND_X11LOCK_H_ 

#include <unistd.h>
#include <sys/types.h>


struct x11lock {
	pid_t helper_pid;
};

int block_x11_input(struct x11lock *xl);
void unblock_x11_input(struct x11lock *xl);

#endif /* BACKEND_X11LOCK_H_ */
