#ifndef BACKEND_DEVICELOCK_H_
#define BACKEND_DEVICELOCK_H_

#include "probe.h"


struct devicelock {
	void (*destroy)(struct devicelock *s);
	void (*event)(struct devicelock *s);
};

struct devicelock * devicelock_probe();
void devicelock_destroy(struct devicelock *s);

DECLARE_PROBES(devicelock);
#define DEVICELOCK_PROBE(_name, _func)	DEFINE_PROBE(devicelock, _name, _func)

#endif /* BACKEND_DEVICELOCK_H_ */
