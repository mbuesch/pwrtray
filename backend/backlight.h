#ifndef BACKEND_BACKLIGHT_H_
#define BACKEND_BACKLIGHT_H_

#include "timer.h"
#include "api.h"
#include "probe.h"


struct backlight {
	const char *name;

	int (*min_brightness)(struct backlight *b);
	int (*max_brightness)(struct backlight *b);
	int (*brightness_step)(struct backlight *b);
	int (*current_brightness)(struct backlight *b);
	int (*set_brightness)(struct backlight *b, int value);
	int (*screen_lock)(struct backlight *b, int lock);
	int (*screen_is_locked)(struct backlight *b);

	void (*destroy)(struct backlight *b);
	int (*update)(struct backlight *b);
	unsigned int poll_interval;

	/* Internal */
	int autodim_enabled;
	int framebuffer_fd;
	int fb_blanked;
	struct sleeptimer timer;
};

void backlight_init(struct backlight *b, const char *name);

struct backlight * backlight_probe(void);
void backlight_destroy(struct backlight *b);

int backlight_fill_pt_message_stat(struct backlight *b, struct pt_message *msg);
int backlight_notify_state_change(struct backlight *b);

int backlight_set_brightness(struct backlight *b, int value);
int backlight_set_percentage(struct backlight *b, unsigned int percent);
int backlight_screen_lock(struct backlight *b, int lock);

DECLARE_PROBES(backlight);
#define BACKLIGHT_PROBE(_name, _func)	DEFINE_PROBE(backlight, _name, _func)

#endif /* BACKEND_BACKLIGHT_H_ */
