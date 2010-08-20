#include "main.h"
#include "battery.h"
#include "backlight.h"

#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QGridLayout>
#include <QtGui/QCursor>

#include <iostream>

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

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(NULL, "No systray found",
				      "Could not find a systray.");
		return 1;
	}
	TrayIcon icon;
	icon.show();
	if (!icon.init())
		return 1;
	return app.exec();
}

#include "main.moc"
