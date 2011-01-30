#include <QApplication>
#include <QtGui>

#include "Controller.h"
#include "Transmitter.h"

Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv), transmitter(NULL), window(NULL),
  labelUptime(NULL), labelLoadAvg(NULL), labelWlan(NULL),
  camera_x(0), camera_y(0), motor_right(0), motor_left(0)
{

}



Controller::~Controller(void)
{
  // Delete the transmitter, if any
  if (transmitter) {
	delete transmitter;
	transmitter = NULL;
  }

  // Delete window, if any
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

  QPushButton *button = new QPushButton("Send ping");
  QObject::connect(button, SIGNAL(clicked()), transmitter, SLOT(sendPing()));

  // Top level horizontal box
  QHBoxLayout *mainHoriz = new QHBoxLayout();
  window->setLayout(mainHoriz);

  // Vertical box with slider and the camera screen
  QVBoxLayout *screenVert = new QVBoxLayout();
  mainHoriz->addLayout(screenVert);
  
  QSlider *horizSlider = new QSlider(Qt::Horizontal);
  screenVert->addWidget(horizSlider);
  QObject::connect(horizSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraX(int)));

  QFrame *frame = new QFrame(window);
  screenVert->addWidget(frame);

  // Vertical slider next to camera screen
  QSlider *vertSlider = new QSlider(Qt::Vertical);
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

