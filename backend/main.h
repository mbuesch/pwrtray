#ifndef BACKEND_MAIN_H_
#define BACKEND_MAIN_H_

#include "api.h"


void block_signals(void);
void unblock_signals(void);

void notify_clients(struct pt_message *msg, uint16_t flags);

#endif /* BACKEND_MAIN_H_ */
