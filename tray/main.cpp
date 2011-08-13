/*
 *   Copyright (C) 2010-2011 Michael Buesch <m@bues.ch>
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
#include <sys/signal.h>


using namespace std;


TrayWindow::TrayWindow(TrayIcon *_tray)
 : QMenu(NULL)
 , tray (_tray)
 , blockBrightnessChange (false)
 , defaultAutodimMax(100)
{
	struct pt_message m;
	int err;
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

	err = tray->getBackend()->getBacklightState(&m);
	if (err) {
		cerr << "Failed to fetch initial backlight state" << endl;
	} else {
		bool autodim = !!(m.bl_stat.flags & htonl(PT_BL_FLG_AUTODIM));
		brAutoAdj->setCheckState(autodim ? Qt::Checked : Qt::Unchecked);
		defaultAutodimMax = ntohl(m.bl_stat.default_autodim_max_percent);
		if (autodim)
			brightness->setValue(defaultAutodimMax);
		else
			brightness->setValue(ntohl(m.bl_stat.brightness));
		updateBacklightToolTip(autodim);
	}

	connect(brightness, SIGNAL(valueChanged(int)),
		this, SLOT(desiredBrightnessChanged(int)));

	connect(brAutoAdj, SIGNAL(stateChanged(int)),
		this, SLOT(brightnessAutoAdjChanged(int)));
}

TrayWindow::~TrayWindow()
{
}

void TrayWindow::updateBacklightToolTip(bool autodim)
{
	if (autodim)
		tray->setBacklightToolTip("LCD in autodim mode");
	else
		tray->setBacklightToolTip("");
}

void TrayWindow::brightnessAutoAdjChanged(int unused)
{
	bool on = (brAutoAdj->checkState() == Qt::Checked);
	int err;

	if (blockBrightnessChange)
		return;

	if (on) {
		err = tray->getBackend()->setBacklightAutodim(true, -1);
		if (err)
			cerr << "Failed to enable auto-dimming" << endl;
		blockBrightnessChange = true;
		brightness->setValue(defaultAutodimMax);
		blockBrightnessChange = false;
	} else {
		err = tray->getBackend()->setBacklightAutodim(false, 0);
		if (err)
			cerr << "Failed to disable auto-dimming" << endl;
	}
	updateBacklightSlider();
	updateBacklightToolTip(on);
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
	unsigned int minval, maxval, curval;
	unsigned int range, percent;

	if (!msg) {
		err = tray->getBackend()->getBatteryState(&m);
		if (err) {
			cerr << "Failed to fetch battery state" << endl;
			return;
		}
		msg = &m;
	}

	minval = ntohl(msg->bat_stat.min_level);
	maxval = ntohl(msg->bat_stat.max_level);
	curval = ntohl(msg->bat_stat.level);
	if (minval == maxval) {
		/* No batt info */
		minval = 0;
		maxval = 1;
		curval = 0;
		battLabel->setText("No batt info");
	} else {
		if (msg->bat_stat.flags & htonl(PT_BAT_FLG_ONAC))
			battLabel->setText("Batt (AC):");
		else
			battLabel->setText("Batt:");
	}
	battBar->setMinimum(minval);
	battBar->setMaximum(maxval);
	battBar->setValue(curval);

	range = maxval - minval;
	if (range)
		percent = (curval - minval) * 100 / range;
	else
		percent = 0;
	tray->setBatteryToolTip(battLabel->text() +
				QString(" %1%").arg(percent));
}

void TrayWindow::updateBacklightSlider(struct pt_message *msg)
{
	struct pt_message m;
	int err;
	unsigned int step, flags;

	if (!msg) {
		err = tray->getBackend()->getBacklightState(&m);
		if (err) {
			cerr << "Failed to fetch backlight state" << endl;
			return;
		}
		msg = &m;
	}

	blockBrightnessChange = true;
	step = ntohl(msg->bl_stat.brightness_step);
	flags = ntohl(msg->bl_stat.flags);
	brightness->setSingleStep(step);
	brightness->setMinimum(max(step, (unsigned int)ntohl(msg->bl_stat.min_brightness)));
	brightness->setMaximum(ntohl(msg->bl_stat.max_brightness));
	if (!(flags & PT_BL_FLG_AUTODIM))
		brightness->setValue(ntohl(msg->bl_stat.brightness));
	blockBrightnessChange = false;
}

void TrayWindow::desiredBrightnessChanged(int newVal)
{
	bool autodim = (brAutoAdj->checkState() == Qt::Checked);
	int err;

	if (blockBrightnessChange)
		return;

	if (autodim) {
		err = tray->getBackend()->setBacklightAutodim(true, newVal);
		if (err)
			cerr << "Failed to set autodim max" << endl;
	} else {
		tray->getBackend()->setBacklight(newVal);
	}
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

void TrayIcon::setBacklightToolTip(const QString &text)
{
	backlightToolTip = text;
	updateToolTip();
}

void TrayIcon::setBatteryToolTip(const QString &text)
{
	batteryToolTip = text;
	updateToolTip();
}

void TrayIcon::updateToolTip()
{
	QString tt(backlightToolTip);
	if (!tt.isEmpty())
		tt += "\n";
	tt += batteryToolTip;
	setToolTip(tt);
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

	cout << "PwrTray: Waiting for systray ..." << flush;
	while (!QSystemTrayIcon::isSystemTrayAvailable()) {
		if (count++ >= 240) {
			QMessageBox::critical(NULL, "PwrTray: No systray found",
					      "PwrTray: Could not find a systray.");
			return false;
		}
		msleep(500);
		cout << "." << flush;
	}
	cout << " found" << endl;

	return true;
}

static void signal_terminate(int signal)
{
	cout << "Terminating signal received." << endl;
	QApplication::exit(0);
}

static int install_sighandler(int signal, void (*handler)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = handler;

	return sigaction(signal, &act, NULL);
}

static int setup_signal_handlers(void)
{
	int err = 0;

	err |= install_sighandler(SIGINT, signal_terminate);
	err |= install_sighandler(SIGTERM, signal_terminate);

	return err ? -1 : 0;
}

int main(int argc, char **argv)
{
	int err;

	QApplication app(argc, argv);
	err = setup_signal_handlers();
	if (err) {
		cerr << "Failed to initilize signal handlers" << endl;
		return 1;
	}
	if (!waitForSystray())
		return 1;
	TrayIcon icon;
	icon.show();
	if (!icon.init())
		return 1;
	return app.exec();
}

#include "main.moc"
