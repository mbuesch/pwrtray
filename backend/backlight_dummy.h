#ifndef BACKEND_BACKLIGHT_DUMMY_H_
#define BACKEND_BACKLIGHT_DUMMY_H_

#include "backlight.h"

struct backlight_dummy {
	struct backlight backlight;
};

struct backlight * backlight_dummy_probe(void);

#endif /* BACKEND_BACKLIGHT_DUMMY_H_ */
