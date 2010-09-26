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

#include "main.h"
#include "backend.h"

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QGridLayout>
#include <QtGui/QCursor>

#include <iostream>

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>


using namespace std;


TrayWindow::TrayWindow(TrayIcon *_tray)
 : QMenu(NULL)
 , tray (_tray)
{
	QLabel *label;
	QGridLayout *l = new QGridLayout(this);
	setLayout(l);

	label = new QLabel("LCD:", this);
	l->addWidget(label, 0, 0);
	brightness = new QSlider(this);
	brightness->setOrientation(Qt::Horizontal);
	l->addWidget(brightness, 0, 1);

	brAutoAdj = new QCheckBox("Auto brightness", this);
	l->addWidget(brAutoAdj, 1, 1);

	battLabel = new QLabel(this);
	l->addWidget(battLabel, 3, 0);
	battBar = new QProgressBar(this);
	l->addWidget(battBar, 3, 1);

	update();

	connect(brightness, SIGNAL(valueChanged(int)),
		this, SLOT(desiredBrightnessChanged(int)));

	connect(brAutoAdj, SIGNAL(stateChanged(int)),
		this, SLOT(brightnessAutoAdjChanged(int)));
}

TrayWindow::~TrayWindow()
{
}

void TrayWindow::brightnessAutoAdjChanged(int unused)
{
	bool on = (brAutoAdj->checkState() == Qt::Checked);
	int err = tray->getBackend()->setBacklightAutodim(on);
	if (err)
		cerr << "Failed to set auto-dimming" << endl;
	brightness->setEnabled(!on);
}

void TrayWindow::update()
{
	updateBacklightSlider();
	updateBattBar();
}

void TrayWindow::updateBattBar(struct pt_message *msg)
{
	struct pt_message m;
	int err;

	if (!msg) {
		err = tray->getBackend()->getBatteryState(&m);
		if (err) {
			cerr << "Failed to fetch battery state" << endl;
			return;
		}
		msg = &m;
	}

	if (msg->bat_stat.flags & htonl(PT_BAT_FLG_ONAC))
		battLabel->setText("Batt (AC):");
	else
		battLabel->setText("Batt:");
	battBar->setMinimum(ntohl(msg->bat_stat.min_charge));
	battBar->setMaximum(ntohl(msg->bat_stat.max_charge));
	battBar->setValue(ntohl(msg->bat_stat.charge));
}

void TrayWindow::updateBacklightSlider(struct pt_message *msg)
{
	struct pt_message m;
	int err;
	unsigned int step;

	if (!msg) {
		err = tray->getBackend()->getBacklightState(&m);
		if (err) {
			cerr << "Failed to fetch backlight state" << endl;
			return;
		}
		msg = &m;
	}

	step = ntohl(msg->bl_stat.brightness_step);
	brightness->setSingleStep(step);
	brightness->setMinimum(max(step, (unsigned int)ntohl(msg->bl_stat.min_brightness)));
	brightness->setMaximum(ntohl(msg->bl_stat.max_brightness));
	brightness->setValue(ntohl(msg->bl_stat.brightness));
	brAutoAdj->setCheckState((msg->bl_stat.flags & htonl(PT_BL_FLG_AUTODIM)) ?
				 Qt::Checked : Qt::Unchecked);
}

void TrayWindow::desiredBrightnessChanged(int newVal)
{
	tray->getBackend()->setBacklight(newVal);
}

void TrayWindow::showEvent(QShowEvent *event)
{
	QPoint cursor(QCursor::pos());
	int x = cursor.x() - width();
	int y = cursor.y() - height();
	move(max(x, 0), max(y, 0));
	QMenu::showEvent(event);
}

TrayIcon::TrayIcon()
 : window (NULL)
 , backend (NULL)
{
	setIcon(QIcon(stringify(PREFIX) "/share/pwrtray/bulb.png"));
}

TrayIcon::~TrayIcon()
{
	delete window;
	delete backend;
}

bool TrayIcon::init()
{
	int err;

	backend = new Backend();
	err = backend->connectToBackend();
	if (err) {
		delete backend;
		backend = NULL;
		return false;
	}

	window = new TrayWindow(this);

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(wasActivated(QSystemTrayIcon::ActivationReason)));
	connect(backend, SIGNAL(batteryStateChanged(struct pt_message *)),
		this, SLOT(batteryStateChanged(struct pt_message *)));
	connect(backend, SIGNAL(backlightStateChanged(struct pt_message *)),
		this, SLOT(backlightStateChanged(struct pt_message *)));

	return true;
}

void TrayIcon::wasActivated(ActivationReason reason)
{
	window->update();
	window->show();
}

void TrayIcon::batteryStateChanged(struct pt_message *msg)
{
	if (window->isVisible())
		window->updateBattBar(msg);
}

void TrayIcon::backlightStateChanged(struct pt_message *msg)
{
	if (window->isVisible())
		window->updateBacklightSlider(msg);
}

static void msleep(unsigned int msecs)
{
	int err;
	struct timespec time;

	time.tv_sec = 0;
	while (msecs >= 1000) {
		time.tv_sec++;
		msecs -= 1000;
	}
	time.tv_nsec = msecs;
	time.tv_nsec *= 1000000;
	do {
		err = nanosleep(&time, &time);
	} while (err && errno == EINTR);
	if (err) {
		cerr << "nanosleep() failed with: "
		     << strerror(errno) << endl;
	}
}

static bool waitForSystray()
{
	unsigned int count = 0;
	while (!QSystemTrayIcon::isSystemTrayAvailable()) {
		if (count++ >= 100) {
			QMessageBox::critical(NULL, "No systray found",
					      "Could not find a systray.");
			return false;
		}
		msleep(100);
	}
	return true;
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (!waitForSystray())
		return 1;
	TrayIcon icon;
	icon.show();
	if (!icon.init())
		return 1;
	return app.exec();
}

#include "main.moc"
