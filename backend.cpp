/*
 *   Copyright (C) 2010 Michael Buesch <mb@bu3sch.de>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "backend.h"
#include "backend/api.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

#include <QApplication>

using namespace std;


Backend::Backend()
 : fd (-1)
 , notifier (NULL)
 , errcount (0)
{
}

Backend::~Backend()
{
	if (fd != -1)
		close(fd);
}

int Backend::connectToBackend()
{
	struct sockaddr_un sockaddr;
	int err;
	struct pt_message msg;

	if (fd >= 0)
		return 0;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		cerr << "Failed to create backend socket: "
		     << strerror(errno) << endl;
		goto error;
	}
	sockaddr.sun_family = AF_UNIX;
	strncpy(sockaddr.sun_path, PT_SOCKET, sizeof(sockaddr.sun_path) - 1);
	err = ::connect(fd, (struct sockaddr *)&sockaddr, SUN_LEN(&sockaddr));
	if (err) {
		cerr << "Failed to connect to backend socket: "
		     << strerror(errno) << endl;
		goto err_close;
	}

	memset(&msg, 0, sizeof(msg));
	msg.id = htons(PTREQ_PING);
	err = sendMessageSyncReply(&msg);
	if (err) {
		cerr << "Failed to send ping to backend" << endl;
		goto err_close;
	}

	cout << "Connected to backend" << endl;

	notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
	connect(notifier, SIGNAL(activated(int)),
		this, SLOT(readNotification(int)));

	memset(&msg, 0, sizeof(msg));
	msg.id = htons(PTREQ_WANT_NOTIFY);
	msg.flags = htons(PT_FLG_ENABLE);
	err = sendMessageSyncReply(&msg);
	if (err) {
		cerr << "Failed to enable backend notifications" << endl;
		goto err_free_noti;
	}

	return 0;

err_free_noti:
	delete notifier;
	notifier = NULL;
err_close:
	close(fd);
	fd = -1;
error:
	return -1;
}

int Backend::sendMessage(struct pt_message *msg)
{
	ssize_t count;

	msg->flags |= htons(PT_FLG_OK);
	count = ::write(fd, msg, sizeof(*msg));
	if (count != sizeof(*msg))
		return -1;

	return 0;
}

int Backend::sendMessageSyncReply(struct pt_message *msg)
{
	uint16_t id = msg->id;
	int err;

	err = sendMessage(msg);
	if (err)
		return err;
	while (1) {
		err = recvMessage(msg);
		if (err)
			return err;
		if ((msg->id != id) || !(msg->flags & htons(PT_FLG_REPLY))) {
			//FIXME better queue it up instead of delivering it synchronously.
			processReceivedMessage(msg);
			continue;
		}
		break;
	}
	if (!(msg->flags & htons(PT_FLG_OK)))
		return -ETXTBSY;

	return 0;
}

int Backend::recvMessage(struct pt_message *msg)
{
	ssize_t count;
	size_t pos = 0;

	memset(msg, 0, sizeof(*msg));
	do {
		count = ::read(fd, ((uint8_t *)msg) + pos, sizeof(*msg) - pos);
		if (count < 0)
			return -1;
		pos += count;
	} while (count != 0 && pos != sizeof(*msg));
	if (pos != sizeof(*msg))
		return -1;

	return 0;
}

void Backend::processReceivedMessage(struct pt_message *msg)
{
	switch (ntohs(msg->id)) {
	case PTNOTI_SRVDOWN:
		cout << "Backend server is going down." << endl;
		QApplication::instance()->exit(1);
		break;
	case PTNOTI_BL_CHANGED:
		emit backlightStateChanged(msg);
		break;
	case PTNOTI_BAT_CHANGED:
		emit batteryStateChanged(msg);
		break;
	default:
		cerr << "Received unknown message: " << ntohs(msg->id) << endl;
	}
}

void Backend::checkErrorCount()
{
	if (errcount < 25)
		return;
	cerr << "Too many backend errors. Is the backend dead?" << endl;
	QApplication::instance()->exit(1);
}

void Backend::readNotification(int sock)
{
	struct pt_message msg;
	int err;

	err = recvMessage(&msg);
	if (err) {
		cerr << "Got read notification, but failed to read message" << endl;
		errcount++;
		checkErrorCount();
		return;
	}
	errcount = 0;
	processReceivedMessage(&msg);
}

int Backend::getBatteryState(struct pt_message *msg)
{
	int err;

	memset(msg, 0, sizeof(*msg));
	msg->id = htons(PTREQ_BAT_GETSTATE);
	err = sendMessageSyncReply(msg);

	return err;
}

int Backend::getBacklightState(struct pt_message *msg)
{
	int err;

	memset(msg, 0, sizeof(*msg));
	msg->id = htons(PTREQ_BL_GETSTATE);
	err = sendMessageSyncReply(msg);

	return err;
}

int Backend::setBacklight(int value)
{
	struct pt_message msg;

	memset(&msg, 0, sizeof(msg));
	msg.id = htons(PTREQ_BL_SETBRIGHTNESS);
	msg.bl_set.brightness = value;

	return sendMessageSyncReply(&msg);
}

int Backend::setBacklightAutodim(bool enable)
{
	struct pt_message msg;

	memset(&msg, 0, sizeof(msg));
	msg.id = htons(PTREQ_BL_AUTODIM);
	msg.flags = htons(PT_FLG_ENABLE);

	return sendMessageSyncReply(&msg);
}

#include "backend.moc"