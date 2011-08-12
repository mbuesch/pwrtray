#ifndef BACKEND_X11LOCK_H_ 
#define BACKEND_X11LOCK_H_ 

#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>


struct x11lock {
	sig_atomic_t helper_pid;
	sig_atomic_t killed;
};

int block_x11_input(struct x11lock *xl);
void unblock_x11_input(struct x11lock *xl);
void x11lock_sigchld(struct x11lock *xl, int wait);

#endif /* BACKEND_X11LOCK_H_ */
