/*
 *   Copyright (C) 2010-2015 Michael Buesch <m@bues.ch>
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
#include "util.h"

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QGridLayout>
#include <QtGui/QCursor>

#include <iostream>

#include <stdint.h>
#include <sys/signal.h>


using namespace std;


TrayWindow::TrayWindow(TrayIcon *_tray)
 : QMenu(NULL)
 , tray (_tray)
 , blockBrightnessChange (false)
 , realBrightnessMinVal (0)
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
	l->addWidget(brightness, 0, 1, 1, 2);

	brAutoAdj = new QCheckBox("Auto", this);
	l->addWidget(brAutoAdj, 1, 1);

	brAutoAdjAC = new QCheckBox("Auto on AC", this);
	l->addWidget(brAutoAdjAC, 1, 2);

	battLabel = new QLabel(this);
	l->addWidget(battLabel, 3, 0);
	battBar = new QProgressBar(this);
	l->addWidget(battBar, 3, 1, 1, 2);

	update();

	err = tray->getBackend()->getBacklightState(&m);
	if (err) {
		cerr << "Failed to fetch initial backlight state" << endl;
	} else {
		bool autodim = !!(m.bl_stat.flags & htonl(PT_BL_FLG_AUTODIM));
		bool autoac = !!(m.bl_stat.flags & htonl(PT_BL_FLG_AUTODIM_AC));
		brAutoAdj->setCheckState(autodim ? Qt::Checked : Qt::Unchecked);
		brAutoAdjAC->setCheckState(autoac ? Qt::Checked : Qt::Unchecked);
		brightness->setValue(ntohl(m.bl_stat.brightness));
		//TODO slider min/max?
		updateBacklightToolTip(autodim);
	}

	connect(brightness, SIGNAL(valueChanged(int)),
		this, SLOT(desiredBrightnessChanged(int)));

	connect(brAutoAdj, SIGNAL(stateChanged(int)),
		this, SLOT(brightnessAutoAdjChanged(int)));
	connect(brAutoAdjAC, SIGNAL(stateChanged(int)),
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
	bool on_ac = (brAutoAdjAC->checkState() == Qt::Checked);
	int err;
	int minval, maxval, range, val;

	if (blockBrightnessChange)
		return;

	if (on) {
		/* Auto-adjust on */
		minval = realBrightnessMinVal;
		maxval = brightness->maximum();
		range = maxval - minval;
		val = brightness->value();
		if (range) {
			val = div_round(static_cast<int64_t>(val) * 100,
					static_cast<int64_t>(range));
		} else
			val = 0;
		err = tray->getBackend()->setBacklightAutodim(true, on_ac, val);
		if (err)
			cerr << "Failed to enable auto-dimming" << endl;
		brAutoAdjAC->setEnabled(true);
	} else {
		/* Auto-adjust off */
		err = tray->getBackend()->setBacklightAutodim(false, on_ac, 0);
		if (err)
			cerr << "Failed to disable auto-dimming" << endl;
		brAutoAdjAC->setEnabled(false);
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
	QString battText("No bat. info");

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
	} else {
		battText = "Bat.";
		if (msg->bat_stat.flags & htonl(PT_BAT_FLG_ACUNKNOWN))
			battText += " / AC?";
		else if (msg->bat_stat.flags & htonl(PT_BAT_FLG_ONAC))
			battText += " / AC";
		if (msg->bat_stat.flags & htonl(PT_BAT_FLG_CHUNKNOWN))
			battText += " / charg.?";
		else if (msg->bat_stat.flags & htonl(PT_BAT_FLG_CHARGING))
			battText += " / charg.";
		battText += ":";
	}
	battBar->setMinimum(minval);
	battBar->setMaximum(maxval);
	battBar->setValue(curval);
	battLabel->setText(battText);

	range = maxval - minval;
	if (range)
		percent = div_round(static_cast<uint64_t>(curval - minval) * 100,
				    static_cast<uint64_t>(range));
	else
		percent = 0;
	tray->setBatteryToolTip(QString("%1 %2%").arg(battText).arg(percent));
}

void TrayWindow::updateBacklightSlider(struct pt_message *msg)
{
	struct pt_message m;
	int err;
	unsigned int step, flags, val, minval, maxval;

	if (!msg) {
		err = tray->getBackend()->getBacklightState(&m);
		if (err) {
			cerr << "Failed to fetch backlight state" << endl;
			return;
		}
		msg = &m;
	}

	step = ntohl(msg->bl_stat.brightness_step);
	flags = ntohl(msg->bl_stat.flags);
	val = ntohl(msg->bl_stat.brightness);
	minval = ntohl(msg->bl_stat.min_brightness);
	maxval = ntohl(msg->bl_stat.max_brightness);
	/* Force minval to >= 1%, but save the original minval. */
	realBrightnessMinVal = minval;
	if (flags & PT_BL_FLG_AUTODIM)
		minval = max(minval, 1u);
	else
		minval = div_round((maxval - minval) * 1, 100) + minval;
	minval = round_up(minval, step);

	blockBrightnessChange = true;

	brightness->setSingleStep(step);
	brightness->setMinimum(minval);
	brightness->setMaximum(maxval);
	brightness->setValue(val);

	blockBrightnessChange = false;
}

void TrayWindow::desiredBrightnessChanged(int newVal)
{
	bool autodim = (brAutoAdj->checkState() == Qt::Checked);
	bool autodim_ac = (brAutoAdjAC->checkState() == Qt::Checked);
	int err;

	if (blockBrightnessChange)
		return;

	if (autodim) {
		err = tray->getBackend()->setBacklightAutodim(true, autodim_ac, newVal);
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
	window->show();
}

void TrayIcon::batteryStateChanged(struct pt_message *msg)
{
	window->updateBattBar(msg);
}

void TrayIcon::backlightStateChanged(struct pt_message *msg)
{
	window->updateBacklightSlider(msg);
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

#include "moc/main.moc"
