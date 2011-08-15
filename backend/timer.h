#ifndef SLEEPTIMER_H_
#define SLEEPTIMER_H_

#include "list.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


struct sleeptimer;

typedef void (*sleeptimer_callback_t)(struct sleeptimer *timer);

struct sleeptimer {
	const char *name;
	struct timespec timeout;
	sleeptimer_callback_t callback;
	unsigned int id;
	struct list_head list;
};

void sleeptimer_init(struct sleeptimer *timer,
		     const char *name,
		     sleeptimer_callback_t callback);
void sleeptimer_set_timeout_relative(struct sleeptimer *timer,
				     unsigned int msecs);

void sleeptimer_enqueue(struct sleeptimer *timer);
void sleeptimer_dequeue(struct sleeptimer *timer);

int sleeptimer_wait_next(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TIMER_H_ */
