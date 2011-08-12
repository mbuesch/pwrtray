#ifndef BACKEND_XEVREP_H_ 
#define BACKEND_XEVREP_H_ 

#include <unistd.h>
#include <sys/types.h>


struct xevrep {
	pid_t helper_pid;
};

int xevrep_enable(struct xevrep *xr);
void xevrep_disable(struct xevrep *xr);

#endif /* BACKEND_XEVREP_H_ */
