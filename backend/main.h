#ifndef BACKEND_MAIN_H_
#define BACKEND_MAIN_H_

#include "api.h"


void block_signals(void);
void unblock_signals(void);

void notify_clients(struct pt_message *msg, uint16_t flags);

void autodim_may_suspend(void);
void autodim_may_resume(void);

#endif /* BACKEND_MAIN_H_ */
