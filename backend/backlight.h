#ifndef BACKEND_BACKLIGHT_H_
#define BACKEND_BACKLIGHT_H_

#include "api.h"


struct backlight {
	int (*min_brightness)(struct backlight *b);
	int (*max_brightness)(struct backlight *b);
	int (*brightness_step)(struct backlight *b);
	int (*current_brightness)(struct backlight *b);
	int (*set_brightness)(struct backlight *b, int value);
	int (*screen_lock)(struct backlight *b, int lock);
	int (*screen_is_locked)(struct backlight *b);

	void (*destroy)(struct backlight *b);

	/* Internal */
	int autodim_enabled;
	int framebuffer_fd;
	int fb_blanked;
};

void backlight_init(struct backlight *b);

struct backlight * backlight_probe(void);
void backlight_destroy(struct backlight *b);

int backlight_fill_pt_message_stat(struct backlight *b, struct pt_message *msg);
int backlight_notify_state_change(struct backlight *b);

int backlight_set_brightness(struct backlight *b, int value);
int backlight_set_percentage(struct backlight *b, unsigned int percent);
int backlight_screen_lock(struct backlight *b, int lock);

#endif /* BACKEND_BACKLIGHT_H_ */
