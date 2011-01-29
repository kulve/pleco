#include <QCoreApplication>

#include "Transmitter.h"
#include "Slave.h"

Slave::Slave(int &argc, char **argv):
  QCoreApplication(argc, argv), transmitter(NULL)
{
}



Slave::~Slave()
{ 

  if (transmitter) {
	delete transmitter;
	transmitter = NULL;
  }

}



void Slave::connect(QString host, quint16 port)
{

  if (transmitter) {
	delete transmitter;
  }

  transmitter = new Transmitter(host, port);

  QTimer *cpuTimer = new QTimer();
  QObject::connect(cpuTimer, SIGNAL(timeout()), this, SLOT(sendCPULoad()));
  cpuTimer->start(1000);
}



void Slave::sendCPULoad(void)
{
  int percent = 5;

  qDebug() << "Sending CPU load:" << percent;

  transmitter->sendCPULoad(percent);
}
