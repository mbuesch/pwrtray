#ifndef BACKEND_BACKLIGHT_OMAPFB_H_
#define BACKEND_BACKLIGHT_OMAPFB_H_

#include "backlight.h"

struct backlight_omapfb {
	struct backlight backlight;

	struct fileaccess *level_file;

	int max_level;
	int current_level;
	int locked;
};

#endif /* BACKEND_BACKLIGHT_OMAPFB_H_ */
