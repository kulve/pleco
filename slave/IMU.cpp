#include "IMU.h"
#include "Hardware.h"

#include <QObject>
#include <QDebug>

IMU::IMU(Hardware *hardware):
  QObject(), hardware(hardware), enabled(false)
{
  // FIXME: how to do this more nicely?
  accRaw[0] = accRaw[1] = accRaw[2] = 0;
  gyroRaw[0] = gyroRaw[1] = gyroRaw[2] = 0;
  magnRaw[0] = magnRaw[1] = magnRaw[2] = 0;

  acc[0] = acc[1] = acc[2] = 0;
  gyro[0] = gyro[1] = gyro[2] = 0;
  magn[0] = magn[1] = magn[2] = 0;

  QObject::connect(hardware, SIGNAL(IMURaw(int accuracy_bytes, int data[9])),
				   this, SLOT(doIMUCalc(int accuracy_bytes, int data[9])));
}



IMU::~IMU()
{ 
  // Disable IMU, if still running
  if (enabled && !hardware->enableIMU(false)) {
	qWarning("Failed to stop IMU");
  }
}



bool IMU::enable(bool enable)
{

  qDebug() << "In" << __FUNCTION__ << ", Enable:" << enable;

  if (!hardware) {
	qWarning("No hardware plugin set!");
	return false;
  }

  // Enable IMU
  if (enable) {

	// Do nothing, if already enabled
	if (enabled) {
	  return true;
	}
	
	if (!hardware->initIMU()) {
	  qWarning("Failed to initialize IMU");
	  return false;
	}
	
	hardware->enableIMU(true);
	enabled = true;
	return true;
  }

  // Disable IMU

  // Do nothing if already disabled
  if (!enabled) {
	return true;
  }

  if (!hardware->enableIMU(false)) {
	qWarning("Failed to stop IMU");
	return false;
  }

  enabled = false;

  return true;
}



QByteArray *IMU::get9DoF(int accuracy_bytes)
{
  qDebug() << "In" << __FUNCTION__ << ", Accuracy in bytes:" << accuracy_bytes;

  QByteArray *data;

  switch (accuracy_bytes) {
  case 1:
	data = new QByteArray(9, 0); // one byte for each 9DoF value
	(*data)[0] = (qint8)acc[0];
	(*data)[1] = (qint8)acc[1];
	(*data)[2] = (qint8)acc[2];
	(*data)[3] = (qint8)gyro[0];
	(*data)[4] = (qint8)gyro[1];
	(*data)[5] = (qint8)gyro[2];
	(*data)[6] = (qint8)magn[0];
	(*data)[7] = (qint8)magn[1];
	(*data)[8] = (qint8)magn[2];
	return data;
  case 2:
	qWarning("2 byte accuracy not implemented!");
	return NULL;
  case 4:
	qWarning("4 byte accuracy not implemented!");
	return NULL;
  default:
	qWarning("Unknown byte accuracy: %d", accuracy_bytes);
	return NULL;
  }
}




QByteArray *IMU::get9DoFRaw(int accuracy_bytes)
{
  qDebug() << "In" << __FUNCTION__ << ", Accuracy in bytes:" << accuracy_bytes;

  QByteArray *data;

  switch (accuracy_bytes) {
  case 1:
	data = new QByteArray(9, 0); // one byte for each 9DoF value
	(*data)[0] = (qint8)accRaw[0];
	(*data)[1] = (qint8)accRaw[1];
	(*data)[2] = (qint8)accRaw[2];
	(*data)[3] = (qint8)gyroRaw[0];
	(*data)[4] = (qint8)gyroRaw[1];
	(*data)[5] = (qint8)gyroRaw[2];
	(*data)[6] = (qint8)magnRaw[0];
	(*data)[7] = (qint8)magnRaw[1];
	(*data)[8] = (qint8)magnRaw[2];
	return data;
  case 2:
	qWarning("2 byte accuracy not implemented!");
	return NULL;
  case 4:
	qWarning("4 byte accuracy not implemented!");
	return NULL;
  default:
	qWarning("Unknown byte accuracy: %d", accuracy_bytes);
	return NULL;
  }
}


void IMU::doIMUCalc(int accuracy_bytes, int data[9])
{
  qDebug() << "In" << __FUNCTION__ << ", Accuracy in bytes:" << accuracy_bytes;

  // FIXME: do the math

  // Raw values

  // Accelerometer data
  accRaw[0] = data[0];
  accRaw[1] = data[1];
  accRaw[2] = data[2];

  qDebug() << "In" << __FUNCTION__ << ", acc:" << accRaw[0] << accRaw[1] << accRaw[2];

  // Gyroscope data
  gyroRaw[0] = data[3];
  gyroRaw[1] = data[4];
  gyroRaw[2] = data[5];

  // Magnetometer data
  magnRaw[0] = data[6];
  magnRaw[1] = data[7];
  magnRaw[2] = data[8];

  // Filtered values

  // Accelerometer data
  acc[0] = data[0];
  acc[1] = data[1];
  acc[2] = data[2];

  // Gyroscope data
  gyro[0] = data[3];
  gyro[1] = data[4];
  gyro[2] = data[5];

  // Magnetometer data
  magn[0] = data[6];
  magn[1] = data[7];
  magn[2] = data[8];
}
