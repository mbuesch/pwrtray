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
	void updateBacklightToolTip(bool autodim);

protected slots:
	void desiredBrightnessChanged(int newVal);
	void brightnessAutoAdjChanged(int unused);

protected:
	int max_autodim;
	TrayIcon *tray;
	QProgressBar *battBar;
	QLabel *battLabel;
	QSlider *brightness;
	QCheckBox *brAutoAdj;
	bool blockBrightnessChange;
	int defaultAutodimMax;
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

	void setBacklightToolTip(const QString &text);
	void setBatteryToolTip(const QString &text);

protected slots:
	void wasActivated(QSystemTrayIcon::ActivationReason reason);
	void batteryStateChanged(struct pt_message *msg);
	void backlightStateChanged(struct pt_message *msg);

protected:
	void updateToolTip();

protected:
	TrayWindow *window;
	Backend *backend;
	QString backlightToolTip;
	QString batteryToolTip;
};

#endif /* MAIN_H_ */
