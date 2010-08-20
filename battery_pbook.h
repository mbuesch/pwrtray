#ifndef BATTERY_PBOOK_H_
#define BATTERY_PBOOK_H_

#include "fileaccess.h"
#include "battery.h"


class BatteryPbook : public Battery
{
	Q_OBJECT
public:
	BatteryPbook(ProcFsFile *info, ProcFsFile *stat);
	virtual ~BatteryPbook();

	static BatteryPbook * probe();

	virtual bool onAC();
	virtual int maxCharge();
	virtual int currentCharge();

protected:
	void update();

protected:
	ProcFsFile *infoFile;
	ProcFsFile *statFile;
	bool ac;
	unsigned int maxChg;
	unsigned int curChg;
};

#endif /* BATTERY_PBOOK_H_ */
