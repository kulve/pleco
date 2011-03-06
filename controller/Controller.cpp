#include <QApplication>
#include <QtGui>

#include "Controller.h"
#include "Transmitter.h"
#include "VideoReceiver.h"

Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv), transmitter(NULL), vr(NULL), window(NULL),
  labelUptime(NULL), labelLoadAvg(NULL), labelWlan(NULL),
  horizSlider(NULL), vertSlider(NULL),
  buttonEnableVideo(NULL), comboboxVideoSource(NULL),
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
  window->resize(800, 600);
  // Vertical box with slider and the camera screen
  QVBoxLayout *screenVert = new QVBoxLayout();
  mainHoriz->addLayout(screenVert);
  
  horizSlider = new QSlider(Qt::Horizontal);
  horizSlider->setMinimum(-90);
  horizSlider->setMaximum(+90);
  horizSlider->setSliderPosition(0);
  QObject::connect(horizSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraX(int)));
  screenVert->addWidget(horizSlider);

  vr = new VideoReceiver(window);
  screenVert->addWidget(vr);

  // Vertical slider next to camera screen
  vertSlider = new QSlider(Qt::Vertical);
  vertSlider->setMinimum(-90);
  vertSlider->setMaximum(+90);
  vertSlider->setSliderPosition(0);
  mainHoriz->addWidget(vertSlider);
  QObject::connect(vertSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraY(int)));

  // Grid layoyt for the slave stats
  QGridLayout *grid = new QGridLayout();
  mainHoriz->addLayout(grid);

  // Stats in separate horizontal boxes
  QLabel *label = NULL;

  // Uptime 
  label = new QLabel("Uptime:");
  labelUptime = new QLabel("");

  grid->addWidget(label, 0, 0, Qt::AlignLeft);
  grid->addWidget(labelUptime, 0, 1, Qt::AlignLeft);

  // Load Avg
  label = new QLabel("Load Avg:");
  labelLoadAvg = new QLabel("");

  grid->addWidget(label, 1, 0, Qt::AlignLeft);
  grid->addWidget(labelLoadAvg, 1, 1, Qt::AlignLeft);

  // Wlan
  label = new QLabel("Wlan:");
  labelWlan = new QLabel("");

  grid->addWidget(label, 2, 0, Qt::AlignLeft);
  grid->addWidget(labelWlan, 2, 1, Qt::AlignLeft);

  // Enable video
  label = new QLabel("Video:");
  grid->addWidget(label, 3, 0, Qt::AlignLeft);
  buttonEnableVideo = new QPushButton("Enable");
  buttonEnableVideo->setCheckable(true);
  QObject::connect(buttonEnableVideo, SIGNAL(clicked(bool)), this, SLOT(clickedEnableVideo(bool)));
  grid->addWidget(buttonEnableVideo, 3, 1, Qt::AlignLeft);

  // Video source
  label = new QLabel("Video source:");
  grid->addWidget(label, 4, 0, Qt::AlignLeft);
  comboboxVideoSource = new QComboBox();
  comboboxVideoSource->addItem("Test");
  comboboxVideoSource->addItem("Camera");
  grid->addWidget(comboboxVideoSource, 4, 1, Qt::AlignLeft);
  QObject::connect(comboboxVideoSource, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedVideoSource(int)));

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
  QObject::connect(transmitter, SIGNAL(media(QByteArray *)), vr, SLOT(consumeVideo(QByteArray *)));

  QObject::connect(vr, SIGNAL(pos(double, double)), this, SLOT(updateCamera(double, double)));
  QObject::connect(vr, SIGNAL(motorControlEvent(QKeyEvent *)), this, SLOT(updateMotor(QKeyEvent *)));

  // Send ping every second (unless other high priority packet are sent)
  //transmitter->enableAutoPing(true);

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



void Controller::updateCamera(double x_percent, double y_percent)
{
  cameraX = (int)(180 * x_percent - 90);
  cameraY = (int)(180 * y_percent - 90);

  //qDebug() << "in" << __FUNCTION__ << ", degrees (X Y):" << x_degree << y_degree;

  horizSlider->setSliderPosition(cameraX);
  vertSlider->setSliderPosition(cameraY);

  prepareSendCameraAndSpeed();
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
  
  qDebug() << "Sending CAS:" << cameraX << cameraY << motorRight << motorLeft;
  transmitter->sendCameraAndSpeed(cameraX, cameraY, motorRight, motorLeft);
}



void Controller::updateMotor(QKeyEvent *event)
{

  int left = motorLeft;
  int right = motorRight;

  // We don't support reverse
  switch(event->key()) {
  case Qt::Key_0:
	left = 0;
	right = 0;
	break;
  case Qt::Key_1:
	left = 10;
	right = 10;
	break;
  case Qt::Key_2:
	left = 20;
	right = 20;
	break;
  case Qt::Key_3:
	left = 30;
	right = 30;
	break;
  case Qt::Key_4:
	left = 40;
	right = 40;
	break;
  case Qt::Key_5:
	left = 50;
	right = 50;
	break;
  case Qt::Key_6:
	left = 60;
	right = 60;
	break;
  case Qt::Key_7:
	left = 70;
	right = 70;
	break;
  case Qt::Key_8:
	left = 80;
	right = 80;
	break;
  case Qt::Key_9:
	left = 90;
	right = 90;
	break;
  case Qt::Key_W:
	left += 10;
	right += 10;
	if (left > 100) left = 100;
	if (right > 100) right = 100;
	break;
  case Qt::Key_S:
	left -= 10;
	right -= 10;
	if (left < 0) left = 0;
	if (right < 0) right = 0;
	break;
  case Qt::Key_A:
	left -= 10;
	right += 10;
	if (left < 0) left = 0;
	if (right > 100) right = 100;
	break;
  case Qt::Key_D:
	left += 10;
	right -= 10;
	if (left > 100) left = 100;
	if (right < 0) right = 0;
	break;
  default:
    qWarning("Unhandled key: %d", event->key());
  }

  if (left != motorLeft || right != motorRight) {
	motorLeft = left;
	motorRight = right;
	prepareSendCameraAndSpeed();
  }
}



void Controller::clickedEnableVideo(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonEnableVideo->isChecked();

  if (buttonEnableVideo->isChecked()) {
	buttonEnableVideo->setChecked(true);
	buttonEnableVideo->setText("Disable");
	
	transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 1);
  } else {
	buttonEnableVideo->setChecked(false);
	buttonEnableVideo->setText("Enable");

	transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 0);
  }

}



void Controller::selectedVideoSource(int index)
{
  qDebug() << "in" << __FUNCTION__ << ", index:" << index;

  transmitter->sendValue(MSG_SUBTYPE_VIDEO_SOURCE, (quint16)index);
}
