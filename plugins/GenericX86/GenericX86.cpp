#include "GenericX86.h"

#include <QtPlugin>
#include <QDebug>



GenericX86::GenericX86(void):
encoderName("ffenc_h263"), dataTimer(NULL)
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



bool GenericX86::initIMU(void)
{
  // No generic IMU
  return true;
}



bool GenericX86::enableIMU(bool enable)
{
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
  int *data;
  int accuracy_bytes = 1;

  
  data = new int[9];

  // No generic IMU, provide dummy 8bit data
  for (int i = 0; i < 9; i++) {
	data[i] = i*10;
  }

  emit(IMURaw(accuracy_bytes, data));
}
