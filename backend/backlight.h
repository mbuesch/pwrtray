#ifndef BACKEND_BACKLIGHT_H_
#define BACKEND_BACKLIGHT_H_

#include "timer.h"
#include "api.h"


struct backlight {
	int (*min_brightness)(struct backlight *b);
	int (*max_brightness)(struct backlight *b);
	int (*brightness_step)(struct backlight *b);
	int (*current_brightness)(struct backlight *b);
	int (*set_brightness)(struct backlight *b, int value);

	void (*destroy)(struct backlight *b);
	int (*update)(struct backlight *b);
	unsigned int poll_interval;

	/* Internal */
	struct sleeptimer timer;
};

void backlight_init(struct backlight *b);

struct backlight * backlight_probe(void);
void backlight_destroy(struct backlight *b);

int backlight_fill_pt_message_stat(struct backlight *b, struct pt_message *msg);
int backlight_notify_state_change(struct backlight *b);

int backlight_set_percentage(struct backlight *b, unsigned int percent);

#endif /* BACKEND_BACKLIGHT_H_ */
