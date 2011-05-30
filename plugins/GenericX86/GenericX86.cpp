#include "GenericX86.h"

#include <QtPlugin>
#include <QDebug>



GenericX86::GenericX86(void):
  encoderName("ffenc_h263"), dataTimer(NULL), imu(NULL)
{

}



GenericX86::~GenericX86(void)
{

}



bool GenericX86::init(void)
{

  return true;
}



QString GenericX86::getVideoEncoderName(void) const
{
  qDebug() << __FUNCTION__ << encoderName;
  return encoderName;
}



bool GenericX86::initIMU(IMU *imu)
{
  qDebug() << "in" << __FUNCTION__ << ", imu:" << imu;

  this->imu = imu;
  return true;
}



bool GenericX86::enableIMU(bool enable)
{
  qDebug() << "in" << __FUNCTION__ << ", enable:" << enable;

  if (enable) {
	// Start providing dummy data ten times a second
	if (dataTimer) {
	  delete dataTimer;
	}
	dataTimer = new QTimer(this);
	QObject::connect(dataTimer, SIGNAL(timeout()), this, SLOT(generateData()));
	dataTimer->start(100);
  } else {
	// Stop providing dummy data
	if (dataTimer) {
	  delete dataTimer;
	}
  }

  return true;
}



void GenericX86::generateData(void)
{
  double data[9];
  quint8 raw8bit[9];

  qDebug() << "in" << __FUNCTION__;

  // No generic IMU, provide dummy 8bit "raw" data and converted values as double
  for (int i = 0; i < 9; i++) {
	data[i]    = i * 10;
	raw8bit[i] = i * 10;
  }

  if (imu) {
	imu->pushSensorData(raw8bit, data);
  }
}



Q_EXPORT_PLUGIN2(generic_x86, GenericX86)
