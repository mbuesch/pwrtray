#ifndef MAIN_H_
#define MAIN_H_

#include <QtGui/QApplication>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMenu>
#include <QtGui/QSlider>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QCheckBox>

#define __stringify(x)		#x
#define stringify(x)		__stringify(x)

class TrayIcon;
class Backend;


class TrayWindow : public QMenu
{
	Q_OBJECT
public:
	TrayWindow(TrayIcon *_tray);
	virtual ~TrayWindow();

	void update();
	void updateBattBar(struct pt_message *msg = NULL);
	void updateBacklightSlider(struct pt_message *msg = NULL);

protected:
	virtual void showEvent(QShowEvent *event);

protected slots:
	void desiredBrightnessChanged(int newVal);
	void brightnessAutoAdjChanged(int unused);

protected:
	TrayIcon *tray;
	QProgressBar *battBar;
	QLabel *battLabel;
	QSlider *brightness;
	QCheckBox *brAutoAdj;
};

class TrayIcon : public QSystemTrayIcon
{
	Q_OBJECT
public:
	TrayIcon();
	virtual ~TrayIcon();

	bool init();

	Backend * getBackend()
		{ return backend; }

protected slots:
	void wasActivated(QSystemTrayIcon::ActivationReason reason);
	void batteryStateChanged(struct pt_message *msg);
	void backlightStateChanged(struct pt_message *msg);

protected:
	TrayWindow *window;
	Backend *backend;
};

#endif /* MAIN_H_ */
