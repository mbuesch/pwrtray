#ifndef BACKEND_XEVREP_H_ 
#define BACKEND_XEVREP_H_ 

#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>


struct xevrep {
	sig_atomic_t helper_pid;
	sig_atomic_t killed;
};

int xevrep_enable(struct xevrep *xr);
void xevrep_disable(struct xevrep *xr);
void xevrep_sigchld(struct xevrep *xr, int wait);

#endif /* BACKEND_XEVREP_H_ */
