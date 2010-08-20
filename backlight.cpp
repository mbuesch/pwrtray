#include "backlight.h"
#include "backlight_omapfb.h"
#include "backlight_class.h"


Backlight::Backlight()
{
}

Backlight::~Backlight()
{
}

Backlight * Backlight::probe()
{
	Backlight *b;

	b = BacklightClass::probe();
	if (b)
		return b;

	b = BacklightOmapfb::probe();
	if (b)
		return b;

	return NULL;
}

int Backlight::minBrightness()
{
	return 0;
}

int Backlight::brightnessStep()
{
	return 1;
}

#include "backlight.moc"
