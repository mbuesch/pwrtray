#ifndef BACKEND_MAIN_H_
#define BACKEND_MAIN_H_

#include "api.h"
#include "conf.h"
#include "battery.h"
#include "backlight.h"
#include "devicelock.h"
#include "x11lock.h"
#include "autodim.h"


struct backend {
	struct config_file *config;
	struct battery *battery;
	struct backlight *backlight;
	struct devicelock *devicelock;
	struct x11lock x11lock;
	struct autodim *autodim;
};

extern struct backend backend;

void block_signals(void);
void unblock_signals(void);

void notify_clients(struct pt_message *msg, uint16_t flags);

#endif /* BACKEND_MAIN_H_ */
