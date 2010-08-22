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
#include "battery.h"
#include "backlight.h"

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

	battLabel = new QLabel(this);
	l->addWidget(battLabel, 1, 0);
	battBar = new QProgressBar(this);
	l->addWidget(battBar, 1, 1);

	updateBacklightSlider();
	updateBattBar();

	connect(brightness, SIGNAL(valueChanged(int)),
		this, SLOT(desiredBrightnessChanged(int)));
}

TrayWindow::~TrayWindow()
{
}

void TrayWindow::updateBattBar()
{
	Battery *batt = tray->battery();
	if (batt->onAC())
		battLabel->setText("Batt (AC):");
	else
		battLabel->setText("Batt:");
	battBar->setMinimum(batt->minCharge());
	battBar->setMaximum(batt->maxCharge());
	battBar->setValue(batt->currentCharge());
}

void TrayWindow::updateBacklightSlider()
{
	Backlight *backlight = tray->backlight();
	brightness->setMinimum(backlight->minBrightness());
	brightness->setMaximum(backlight->maxBrightness());
	brightness->setSingleStep(backlight->brightnessStep());
	brightness->setValue(backlight->currentBrightness());
}

void TrayWindow::desiredBrightnessChanged(int newVal)
{
	tray->backlight()->setBrightness(newVal);
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
 , batt (NULL)
 , backl (NULL)
{
	setIcon(QIcon(stringify(PREFIX) "/share/pwrtray/bulb.png"));
}

TrayIcon::~TrayIcon()
{
	delete batt;
	delete backl;
	delete window;
}

bool TrayIcon::init()
{
	batt = Battery::probe();
	if (!batt) {
		QMessageBox::critical(NULL, "Battery init failed",
				      "Failed to initialize battery meter");
		goto error;
	}
	backl = Backlight::probe();
	if (!backl) {
		QMessageBox::critical(NULL, "Backlight init failed",
				      "Failed to initialize backlight");
		goto error;
	}

	window = new TrayWindow(this);

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(wasActivated(QSystemTrayIcon::ActivationReason)));
	connect(batt, SIGNAL(stateChanged()),
		this, SLOT(batteryStateChanged()));

	return true;
error:
	delete batt;
	delete backl;
	batt = NULL;
	backl = NULL;
	return false;
}

void TrayIcon::wasActivated(ActivationReason reason)
{
	window->show();
}

void TrayIcon::batteryStateChanged()
{
	if (window->isVisible())
		window->updateBattBar();
}

void TrayIcon::backlightStateChanged()
{
	if (window->isVisible())
		window->updateBacklightSlider();
}

bool Application::x11EventFilter(XEvent *event)
{
//cout << "XEV" << endl;
	return QApplication::x11EventFilter(event);
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
	Application app(argc, argv);
	if (!waitForSystray())
		return 1;
	TrayIcon icon;
	icon.show();
	if (!icon.init())
		return 1;
	return app.exec();
}

#include "main.moc"
