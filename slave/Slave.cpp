#include "Slave.h"
#include "Transmitter.h"
#include "Motor.h"
#include "VideoSender.h"

#include <QCoreApplication>

#include <stdlib.h>                     /* getenv */

Slave::Slave(int &argc, char **argv):
  QCoreApplication(argc, argv), transmitter(NULL), process(NULL), stats(NULL),
  motor(NULL), vs(NULL)
{
  stats = new QList<int>;
}



Slave::~Slave()
{ 

  // Kill the stats gathering process, if any
  if (process) {
	process->kill();
	delete process;
	process = NULL;
  }

  // Delete the transmitter, if any
  if (transmitter) {
	delete transmitter;
	transmitter = NULL;
  }

  // Delete stats
  if (stats) {
	delete stats;
	stats = NULL;
  }

  // Delete Motor
  if (motor) {
	delete motor;
	motor = NULL;
  }
}



bool Slave::init(void)
{
  if (motor) {
	delete motor;
  }

  // Initialize the motors and shut them down.
  motor = new Motor();
  
  // Init may fail but we ignore it now. It will just disable motors.
  // FIXME: send an error to controller?
  motor->init();

  return true;
}



void Slave::connect(QString host, quint16 port)
{

  // Delete old transmitter if any
  if (transmitter) {
	delete transmitter;
  }

  // Create a new transmitter
  transmitter = new Transmitter(host, port);

  // Connect the incoming data signals
  QObject::connect(transmitter, SIGNAL(cameraX(int)), this, SLOT(updateCameraX(int)));
  QObject::connect(transmitter, SIGNAL(cameraY(int)), this, SLOT(updateCameraY(int)));
  QObject::connect(transmitter, SIGNAL(motorRight(int)), this, SLOT(updateMotorRight(int)));
  QObject::connect(transmitter, SIGNAL(motorLeft(int)), this, SLOT(updateMotorLeft(int)));
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));

  transmitter->initSocket();

  // Launch a process gathering system info and feeding it to this process through stdout
  QString program = "python";
  QStringList arguments;
  arguments << "scripts/get_stats.py";
  
  process = new QProcess(this);
  process->setReadChannel(QProcess::StandardOutput);

  // Connect the get_stats.py child process signals
  QObject::connect(process, SIGNAL(readyRead()), this, SLOT(readStats()));
  QObject::connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
  QObject::connect(process, SIGNAL(started()), this, SLOT(processStarted()));
  QObject::connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));

  process->start(program, arguments);

  // Create and enable sneding video
  if (vs) {
	delete vs;
  }
  vs = new VideoSender();

  QObject::connect(vs, SIGNAL(media(QByteArray*)), transmitter, SLOT(sendMedia(QByteArray*)));
}



void Slave::sendStats(void)
{
  transmitter->sendStats(stats);
}



void Slave::readStats(void)
{
  qDebug() << "in" << __FUNCTION__;

  // Empty stats
  stats->clear();

  // Read all received lines (should be just one though)
  while (process->canReadLine()) {

	// Read stats on one line
	QByteArray line = process->readLine();
	line.chop(1);  // remove new line

	// Split line to individual stats
	QList<QByteArray> list = line.split(' ');
	
	// Go through the stats
	for (int i = 0; i < list.size(); ++i) {

	  // Convert stats to an array of ints
	  // - means "no data", we convert it to 0 for now
	  if (list.at(i) == "-") {
		stats->push_back(0);
	  } else {
		stats->push_back(list.at(i).toInt());
	  }
	}
  }

  // Send the latest stats
  sendStats();
}



void Slave::processError(QProcess::ProcessError error)
{
  qDebug() << "in" << __FUNCTION__ << ": error:" << error;
}



void Slave::processStarted(void)
{
  qDebug() << "in" << __FUNCTION__;
}


void Slave::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  qDebug() << "in" << __FUNCTION__ << ": exitCode:" << exitCode << ", exitStatus:" << exitStatus;
}



void Slave::updateCameraX(int degrees)
{
  qDebug() << "in" << __FUNCTION__ << ", degrees: " << degrees;

  motor->cameraX(degrees);
}



void Slave::updateCameraY(int degrees)
{
  qDebug() << "in" << __FUNCTION__ << ", degrees: " << degrees;

  motor->cameraY(degrees);
}



void Slave::updateMotorRight(int percent)
{
  qDebug() << "in" << __FUNCTION__ << ", percent:" << percent;

  motor->motorRight(percent);
}



void Slave::updateMotorLeft(int percent)
{
  qDebug() << "in" << __FUNCTION__ << ", percent:" << percent;

  motor->motorLeft(percent);
}



void Slave::updateValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << type << ", value:" << value;

  switch (type) {
  case MSG_SUBTYPE_ENABLE_VIDEO:
	vs->enableSending(value?true:false);
	break;
  case MSG_SUBTYPE_VIDEO_SOURCE:
	vs->setVideoSource(value);
	break;
  default:
	qWarning("%s: Unknown type: %d", __FUNCTION__, type);
  }
}
