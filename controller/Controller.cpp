#include <QApplication>
#include <QtGui>

#include "Controller.h"
#include "Transmitter.h"
#include "VideoReceiver.h"

Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv), transmitter(NULL), vr(NULL), window(NULL),
  labelUptime(NULL), labelLoadAvg(NULL), labelWlan(NULL),
  cameraX(0), cameraY(0), motorRight(0), motorLeft(0),
  cameraAndSpeedTimer(NULL), cameraAndSpeedTime(NULL)
{

}



Controller::~Controller(void)
{
  // Delete the transmitter
  if (transmitter) {
	delete transmitter;
	transmitter = NULL;
  }

  // Delete video receiver
  if (vr) {
	delete vr;
	vr = NULL;
  }

  // Delete window
  if (window) {
	delete window;
	window = NULL;
  }

}



void Controller::createGUI(void)
{
  // If old top level window exists, delete it first
  if (window) {
	delete window;
  }
  
  window = new QWidget();
  window->setWindowTitle("Controller");

  // Top level horizontal box
  QHBoxLayout *mainHoriz = new QHBoxLayout();
  window->setLayout(mainHoriz);

  // Vertical box with slider and the camera screen
  QVBoxLayout *screenVert = new QVBoxLayout();
  mainHoriz->addLayout(screenVert);
  
  QSlider *horizSlider = new QSlider(Qt::Horizontal);
  horizSlider->setMinimum(-90);
  horizSlider->setMaximum(+90);
  horizSlider->setSliderPosition(0);
  QObject::connect(horizSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraX(int)));
  screenVert->addWidget(horizSlider);

  vr = new VideoReceiver(window);
  screenVert->addWidget(vr);

  // Vertical slider next to camera screen
  QSlider *vertSlider = new QSlider(Qt::Vertical);
  vertSlider->setMinimum(-90);
  vertSlider->setMaximum(+90);
  vertSlider->setSliderPosition(0);
  mainHoriz->addWidget(vertSlider);
  QObject::connect(vertSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraY(int)));

  // Vertical box for the slave stats
  QVBoxLayout *statsVert = new QVBoxLayout();
  mainHoriz->addLayout(statsVert);

  // Stats in separate horizontal boxes
  QLabel *label = NULL;
  QHBoxLayout *statsHoriz = NULL;
  // Uptime 
  label = new QLabel("Uptime:");
  labelUptime = new QLabel("");
  statsHoriz = new QHBoxLayout();
  statsHoriz->addWidget(label);
  statsHoriz->addWidget(labelUptime);
  statsVert->addLayout(statsHoriz);

  // Load Avg
  label = new QLabel("Load Avg:");
  labelLoadAvg = new QLabel("");
  statsHoriz = new QHBoxLayout();
  statsHoriz->addWidget(label);
  statsHoriz->addWidget(labelLoadAvg);
  statsVert->addLayout(statsHoriz);

  // Wlan
  label = new QLabel("Wlan:");
  labelWlan = new QLabel("");
  statsHoriz = new QHBoxLayout();
  statsHoriz->addWidget(label);
  statsHoriz->addWidget(labelWlan);
  statsVert->addLayout(statsHoriz);

  window->show();
}



void Controller::connect(QString host, quint16 port)
{

  // Delete old transmitter if any
  if (transmitter) {
	delete transmitter;
  }

  // Create a new transmitter
  transmitter = new Transmitter(host, port);

  transmitter->initSocket();

  QObject::connect(transmitter, SIGNAL(rtt(int)), this, SLOT(updateRtt(int)));
  QObject::connect(transmitter, SIGNAL(uptime(int)), this, SLOT(updateUptime(int)));
  QObject::connect(transmitter, SIGNAL(loadAvg(float)), this, SLOT(updateLoadAvg(float)));
  QObject::connect(transmitter, SIGNAL(wlan(int)), this, SLOT(updateWlan(int)));

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);

  // Get ready for receiving video
  vr->enableVideo(true);
}



void Controller::updateRtt(int ms)
{
  qDebug() << "RTT:" << ms;
}



void Controller::updateUptime(int seconds)
{
  qDebug() << "uptime:" << seconds << "seconds";
  if (labelUptime) {
	labelUptime->setText(QString::number(seconds));
  }
}



void Controller::updateLoadAvg(float avg)
{
  qDebug() << "Load avg:" << avg;
  if (labelLoadAvg) {
	labelLoadAvg->setText(QString::number(avg));
  }
}



void Controller::updateWlan(int percent)
{
  qDebug() << "WLAN:" << percent;
  if (labelUptime) {
	labelWlan->setText(QString::number(percent));
  }
}



void Controller::updateCameraX(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  cameraX = degree;

  prepareSendCameraAndSpeed();
}



void Controller::updateCameraY(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  cameraY = degree;

  prepareSendCameraAndSpeed();
}


void Controller::prepareSendCameraAndSpeed(void)
{
  qDebug() << "in" << __FUNCTION__;

  // We don't send CameraAndSpeed messages more often than every 20ms.
  // If <20ms has gone since the last transmission, set a timer for
  // sending after the 20ms has passed.

  // If we have not timers, we haven't sent a packet yet, ever
  if (!cameraAndSpeedTime && !cameraAndSpeedTimer) {
	cameraAndSpeedTime = new QTime();

	// sendCameraAndSpeed() starts the cameAndSpeed stopwatch
	sendCameraAndSpeed();
	return;
  }

  // If we have timer running, we are already sending a message after
  // the 20ms has gone, so do nothing now (the values have already
  // been updated)
  if (cameraAndSpeedTimer) {
	return;
  }

  // No timer for next transmission, but a stopwatch is running. Let's
  // check if 20ms has passed. If passed, send immediately. If not,
  // start a timer for sending once the 20ms has passed.
  if (cameraAndSpeedTime) {

	// 20ms has passed: send a new packet and reset the stopwatch.
	// NOTE: wraps after 24h. Doesn't really matter if we wait extra 20ms in that case.
	int elapsed = cameraAndSpeedTime->elapsed();
	if (elapsed >= 20) {

	  // sendCameraAndSpeed() restarts the cameAndSpeed stopwatch
	  sendCameraAndSpeed();
	} else {

	  // Send timer to send the packet after the 20ms has passed.
	  cameraAndSpeedTimer = new QTimer();
	  cameraAndSpeedTimer->setSingleShot(true);
	  QObject::connect(cameraAndSpeedTimer, SIGNAL(timeout()), this, SLOT(sendCameraAndSpeed()));
	  cameraAndSpeedTimer->start(20 - elapsed);
	}
  }
}



void Controller::sendCameraAndSpeed(void)
{
  qDebug() << "in" << __FUNCTION__;


  // If we have a timer for sending CameraAndSpeed packet, delete it.
  if (cameraAndSpeedTimer) {
	cameraAndSpeedTimer->deleteLater();
	cameraAndSpeedTimer = NULL;
  }

  // If we have timer for calculating time between CameraAndSpeed
  // packets (we should!), restart it now
  if (cameraAndSpeedTime) {
	cameraAndSpeedTime->start();
  }

  transmitter->sendCameraAndSpeed(cameraX, cameraY, motorRight, motorLeft);
}
