#ifndef BACKEND_DEVICELOCK_H_
#define BACKEND_DEVICELOCK_H_

struct devicelock {
	void (*destroy)(struct devicelock *s);
	void (*event)(struct devicelock *s);
};

struct devicelock * devicelock_probe();
void devicelock_destroy(struct devicelock *s);

#endif /* BACKEND_DEVICELOCK_H_ */
