#ifndef BATTERY_H_
#define BATTERY_H_

#include <QtCore/QTimer>


class Battery : public QObject
{
	Q_OBJECT
public:
	Battery(unsigned int pollInterval=0);
	virtual ~Battery();

	static Battery * probe();

	virtual bool onAC();
	virtual int minCharge();
	virtual int maxCharge() = 0;
	virtual int currentCharge() = 0;

	int chargePercentage();

protected:
	virtual void update() { }

protected slots:
	void timeout();

signals:
	void stateChanged();

protected:
	QTimer *timer;
};

#endif /* BATTERY_H_ */
