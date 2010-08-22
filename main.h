#ifndef MAIN_H_
#define MAIN_H_

#include <QtGui/QApplication>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMenu>
#include <QtGui/QSlider>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>

#define __stringify(x)		#x
#define stringify(x)		__stringify(x)

class TrayIcon;
class Battery;
class Backlight;


class TrayWindow : public QMenu
{
	Q_OBJECT
public:
	TrayWindow(TrayIcon *_tray);
	virtual ~TrayWindow();

	void updateBattBar();
	void updateBacklightSlider();

protected:
	virtual void showEvent(QShowEvent *event);

protected slots:
	void desiredBrightnessChanged(int newVal);

protected:
	TrayIcon *tray;
	QProgressBar *battBar;
	QLabel *battLabel;
	QSlider *brightness;
};

class TrayIcon : public QSystemTrayIcon
{
	Q_OBJECT
public:
	TrayIcon();
	virtual ~TrayIcon();

	bool init();

	Battery * battery()
		{ return batt; }

	Backlight * backlight()
		{ return backl; }

protected slots:
	void wasActivated(QSystemTrayIcon::ActivationReason reason);
	void batteryStateChanged();
	void backlightStateChanged();

protected:
	TrayWindow *window;
	Battery *batt;
	Backlight *backl;
};

class Application : public QApplication
{
	Q_OBJECT
public:
	Application(int argc, char **argv)
		 : QApplication (argc, argv)
		{ }
	virtual ~Application()
		{ }

	bool x11EventFilter(XEvent *event);
};

#endif /* MAIN_H_ */
