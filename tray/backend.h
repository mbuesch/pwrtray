#ifndef BACKEND_H_
#define BACKEND_H_

#include "../backend/api.h"

#include <QObject>
#include <QSocketNotifier>


class Backend : public QObject
{
	Q_OBJECT
public:
	Backend();
	virtual ~Backend();

	int connectToBackend();

	int getBatteryState(struct pt_message *msg);
	int getBacklightState(struct pt_message *msg);
	int setBacklight(int value);
	int setBacklightAutodim(bool enable = true);

signals:
	void backlightStateChanged(struct pt_message *msg);
	void batteryStateChanged(struct pt_message *msg);

protected slots:
	void readNotification(int sock);

protected:
	int sendMessage(struct pt_message *msg);
	int sendMessageSyncReply(struct pt_message *msg);
	int recvMessage(struct pt_message *msg);
	void processReceivedMessage(struct pt_message *msg);
	void checkErrorCount();

protected:
	int fd;
	QSocketNotifier *notifier;
	int errcount;
};

#endif /* BACKEND_H_ */
