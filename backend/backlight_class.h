#ifndef BACKEND_BACKLIGHT_CLASS_H_
#define BACKEND_BACKLIGHT_CLASS_H_

#include "backlight.h"

struct backlight_class {
	struct backlight backlight;

	struct fileaccess *actual_br_file;
	struct fileaccess *set_br_file;

	int max_brightness;
	int brightness;
};

#endif /* BACKEND_BACKLIGHT_CLASS_H_ */
