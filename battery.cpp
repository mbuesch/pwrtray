#include "battery.h"
#include "battery_n810.h"
#include "battery_pbook.h"

#include <iostream>

using namespace std;


Battery::Battery(unsigned int pollInterval)
 : timer (NULL)
{
	if (pollInterval) {
		timer = new QTimer(this);
		timer->setInterval(pollInterval);
		connect(timer, SIGNAL(timeout()),
			this, SLOT(timeout()));
	}
}

Battery::~Battery()
{
}

Battery * Battery::probe()
{
	Battery *b;

	b = BatteryPbook::probe();
	if (b)
		return b;

	b = BatteryN810::probe();
	if (b)
		return b;

	return NULL;
}

bool Battery::onAC()
{
	return false;
}

int Battery::minCharge()
{
	return 0;
}

int Battery::chargePercentage()
{
	int minChg = minCharge();
	int maxChg = maxCharge();
	int curChg = currentCharge();
	if (minChg > maxChg) {
		cerr << "WARNING: Min charge value is bigger than max charge value" << endl;
		return 0;
	}
	curChg = max(minChg, curChg);
	curChg = min(maxChg, curChg);
	return curChg * 100 / maxChg;
}

void Battery::timeout()
{
	update();
}

#include "battery.moc"
