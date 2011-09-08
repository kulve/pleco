#include "GumstixOvero.h"

#include <QtPlugin>
#include <QFile>
#include <QTimer>

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>          // errno
#include <string.h>         // strerror

#include <math.h>           // M_PI for heading calculation

// millidegrees/second
#define PLECO_DEG_PER_TICK 0.977

// Length of the message from the SparkFun 6DoF module in binary mode
#define PLECO_6DOF_DATA_LEN 16


GumstixOvero::GumstixOvero(void) :
  imu(NULL), serialFD(-1), serialPortName("/dev/ttyO0"), serialPort(), serialData(), imuRead(false),
  pniFD(-1), pniFileName("/dev/pni11096"), pniDevice(), pniData(), pniRead(false),
  pwm8("/dev/pwm8"), pwm9("/dev/pwm9"), pwm10("/dev/pwm10"), pwm11("/dev/pwm11")
{

  QObject::connect(&serialPort, SIGNAL(readyRead()),
				   this, SLOT(readPendingSerialData()));

  QObject::connect(&pniDevice, SIGNAL(readyRead()),
				   this, SLOT(readPendingPniData()));

  magn[0] = magn[1] = magn[2] = 0;

  for (int i = 0; i < 9; ++i) {
	ins[i] = 0;
	raw8bit[i] = 0;
  }

  // Open motor (PWM) device files
  if (pwm8.exists()) {
	pwm8.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Unbuffered);
  } else {
	qCritical("%s: %s doesn't exist!", __FUNCTION__, pwm8.fileName().toUtf8().data());
  }
  if (pwm9.exists()) {
	pwm9.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Unbuffered);
  } else {
	qCritical("%s: %s doesn't exist!", __FUNCTION__, pwm8.fileName().toUtf8().data());
  }
  if (pwm10.exists()) {
	pwm10.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Unbuffered);
  } else {
	qCritical("%s: %s doesn't exist!", __FUNCTION__, pwm8.fileName().toUtf8().data());
  }
  if (pwm11.exists()) {
	pwm11.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Unbuffered);
  } else {
	qCritical("%s: %s doesn't exist!", __FUNCTION__, pwm8.fileName().toUtf8().data());
  }
}



GumstixOvero::~GumstixOvero(void)
{
  
  // Close serial port
  if (serialFD >= 0) {
	serialPort.close();
	close(serialFD);
  }

  // Close pni device
  if (pniFD >= 0) {
	pniDevice.close();
	close(pniFD);
  }

  // Stop motors and close device files
  if (pwm8.isOpen()) {
	pwm8.close();
  }
  if (pwm9.isOpen()) {
	pwm9.close();
  }
  if (pwm10.isOpen()) {
	pwm10.close();
  }
  if (pwm11.isOpen()) {
	pwm11.close();
  }
}



bool GumstixOvero::init(void)
{

  return true;
}



QString GumstixOvero::getVideoEncoderName(void) const
{
  return "dsph263enc";
}



bool GumstixOvero::initIMU(IMU *imu)
{
  this->imu = imu;

  qDebug() << "in" << __FUNCTION__;

  // We are using SparkFun 6DoF IMU + PNI11096 magnetometer and they
  // don't need any initialisation
  return true;
}



bool GumstixOvero::enableIMU(bool enable)
{

  qDebug() << "in" << __FUNCTION__ << ", enable imu: " << enable;

  // Disable IMU
  if (!enable) {
	
	// Disable 6DoF
	if (serialFD >=0) {
	  serialPort.close();
	  close(serialFD);
	  serialFD = -1;
	}

	// Disable PNI
	if (pniFD >=0) {
	  pniDevice.close();
	  close(pniFD);
	  pniFD = -1;
	}

	return true;
  }

  // Enable IMU
  if (!openSerialDevice()) {
	qCritical("Failed to open and setup serial port");
	return false;
  }

  // Open PNI11096 magnetometer device
  if (!openPniDevice()) {
	qCritical("Failed to open and setup pni device");
	return false;
  }

  return true;
}


void GumstixOvero::setMotor(QFile &pwm, double power)
{
  if(!pwm.isOpen()) {
	return;
  }

  // Clamp values [1,100], 0 shutdowns the PWM signal
  if (power < 1) {
	power = 1;
  } else if (power > 100) {
	power = 100;
  }

  // The pwm kernel module expects percents as 1/10th integer values
  int pwr = (int)(power * 10);
  pwm.write((QString::number(pwr) + "\n").toAscii());
}


void GumstixOvero::setMotorFrontRight(double power)
{
  setMotor(pwm8, power);
}


void GumstixOvero::setMotorFrontLeft(double power)
{
  setMotor(pwm9, power);
}



void GumstixOvero::setMotorRearRight(double power)
{
  setMotor(pwm10, power);
}



void GumstixOvero::setMotorRearLeft(double power)
{
  setMotor(pwm11, power);
}



void GumstixOvero::readPendingSerialData(void)
{
  while (serialPort.bytesAvailable() > 0) {

	// FIXME: use ring buffer?
	serialData.append(serialPort.readAll());
	
	//qDebug() << "in" << __FUNCTION__ << ", data size: " << data.size();
  }

  parseSerialData();
}



void GumstixOvero::readPendingPniData(void)
{
  while (pniDevice.bytesAvailable() > 0) {

	// FIXME: use ring buffer?
	pniData.append(pniDevice.readAll());
	
	//qDebug() << "in" << __FUNCTION__ << ", data size: " << data.size();
  }

  parsePniData();
}



bool GumstixOvero::openSerialDevice(void)
{
  struct termios newtio;
  
  // QFile doesn't support reading UNIX device nodes using readyRead()
  // signal, so we trick around that using TCP socket class.  We'll
  // set up the file descriptor without Qt and then pass the properly
  // set up file descriptor to QTcpSocket for handling the incoming
  // data.
  
  // Open device
  serialFD = open(serialPortName.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK); 
  if (serialFD < 0) { 
	qCritical("Failed to open IMU device: %s", strerror(errno));
	return false;
  }
  
  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
  
  // control mode flags
  newtio.c_cflag = CS8 | CLOCAL | CREAD;
  
  // input mode flags
  newtio.c_iflag = 0;
  
  // output mode flags
  newtio.c_oflag = 0;
  
  // local mode flags
  newtio.c_lflag = 0;
  
  // set input/output speeds
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);
  
  // now clean the serial line and activate the settings
  tcflush(serialFD, TCIFLUSH);
  tcsetattr(serialFD, TCSANOW, &newtio);
  
  // Set the file descriptor for our TCP socket class
  serialPort.setSocketDescriptor(serialFD);
  serialPort.setReadBufferSize(16);

  return true;
}



bool GumstixOvero::openPniDevice(void)
{
  // QFile doesn't support reading UNIX device nodes using readyRead()
  // signal, so we trick around that using TCP socket class.  We'll
  // set up the file descriptor without Qt and then pass the properly
  // set up file descriptor to QTcpSocket for handling the incoming
  // data.
  
  // Open device
  pniFD = open(pniFileName.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK); 
  if (pniFD < 0) { 
	qCritical("Failed to open PNI device: %s", strerror(errno));
	return false;
  }
 
  // Set the file descriptor for our TCP socket class
  pniDevice.setSocketDescriptor(pniFD);
  pniDevice.setReadBufferSize(12); // 3 axis + 3 labels, all 16bit

  return true;
}



void GumstixOvero::parseSerialData(void)
{
  int msgStart = -1;

  if (serialData.size() < PLECO_6DOF_DATA_LEN) {
	// Not enough data
	return;
  }

  // Check if the data buffer has a valid message (it should as we have enough data)
  for (int i = 0; i < serialData.size() - PLECO_6DOF_DATA_LEN; ++i) {
	if (serialData.at(i) == 'A' && serialData.at(i + PLECO_6DOF_DATA_LEN - 1) == 'Z') {
	  msgStart = i;
	  break;
	}
  }

  // No valid data even though we have enough data for one message
  if (msgStart == -1) {

	// If we have over twice the message length of data without a valid message, clear the extra from the start.
	if (serialData.size() > 2 * PLECO_6DOF_DATA_LEN) {
	  serialData.remove(0, serialData.size() - PLECO_6DOF_DATA_LEN);
	}
	return;
  }

  // We have a valid message from the IMU unit starting at msgStart

  // Fill the 16bit 6DoF values after 'A' and 16bit count
  for (int i = 0; i < 6; i++) { 
	int data_i = msgStart + i*2 + 3;
	ins[i] = (serialData[data_i+1] + (serialData[data_i] << 8));
	raw8bit[i] = (quint8)((quint32)(ins[i]) >> 2); /* ignore 2 bits of the 10bit data */
	ins[i] -= 512; // 10bit values to signed
  }
	
  // Just a sanity check
  if (!serialData.at(msgStart + PLECO_6DOF_DATA_LEN - 1) == 'Z') {
	qWarning("Invalid data, skipping: %s", serialData.toHex().data());
	return;
  }

  // Revert accelerometer X and Y values (to get negative values an all axes when pointing towards Earth)
  ins[0] *= -1;
  //ins[1] *= -1;
  raw8bit[0] = 255 - raw8bit[0];
  //raw8bit[1] = 255 - raw8bit[1];

  // Revert gyroscope X and Y values (to get positive values an all axes when rotating according to the right hand rule)
  ins[3] *= -1;
  raw8bit[3] = 255 - raw8bit[3];
  ins[4] *= -1;
  raw8bit[4] = 255 - raw8bit[4];

  /* Convert gyroscope values to degrees per second. Absolute
	 scale of magnetometer and accelerometer doesn't matter */
  ins[3] *= PLECO_DEG_PER_TICK;
  ins[4] *= PLECO_DEG_PER_TICK;
  ins[5] *= PLECO_DEG_PER_TICK;

  // FIXME: use ring buffer?
  // Remove the handled message and the possible garbage before it from the data
  serialData.remove(0, msgStart + PLECO_6DOF_DATA_LEN); 

  imuRead = true;

  // Push data when we have new data from both the IMU and the PNI
  if (imuRead && pniRead) {
	imuRead = false;
	pniRead = false;
	imu->pushSensorData(raw8bit, ins);
  }

}



void GumstixOvero::parsePniData(void)
{
  qint16 data[6];

  if (pniData.size() < 12) {
	qWarning("Buffer doesn't have full data from PNI device, ignoring");
	pniData.clear();
	return;
  }

  memcpy(data, pniData.data(), 12);

  // Verify axies
  if (data[0] != 'X' ||
	  data[2] != 'Y' ||
	  data[4] != 'Z') {
	qWarning("Failed to verify X/Y/Z axis: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x",
			 data[0], data[1], data[2], data[3], data[4], data[5]);
	return;
  }

  // Store magnetometer values as -32767 - 32768
  magn[0] = (qint16)(data[1]);
  magn[1] = (qint16)(data[3]);
  magn[2] = (qint16)(data[5]);
  
  // Revert X and Z axes to match device orientation
  magn[0] *= -1;
  magn[2] *= -1;
  
#ifdef DEBUG_HEADING
  float heading=0;
  float x = magn[0];
  float y = magn[1];
  
  if((x == 0)&&(y < 0)) 
	heading= M_PI/2.0; 
  if((x == 0)&&(y > 0)) 
	heading=3.0*M_PI/2.0; 
  if (x < 0) 
	heading = M_PI - atan(y/x);
  if((x > 0)&&(y < 0)) 
	heading = -atan(y/x); 
  if((x > 0)&&(y > 0)) 
	heading = 2.0*M_PI - atan(y/x);

  heading *= (180 / M_PI);
  
  qDebug() << "in" << __FUNCTION__ << ", values:" << heading << magn[0] << magn[1] << magn[2];
#else
  qDebug() << "in" << __FUNCTION__ << ", values:" << magn[0] << magn[1] << magn[2];
#endif

  // Flag that we have PNI data;
  pniRead = true;

  // Insert last read PNI 11096 magnetometer data
  ins[6] = magn[0]; // PNI x
  ins[7] = magn[1]; // PNI y
  ins[8] = magn[2]; // PNI z
  

  // Convert 16bit values to unsigned 8bit by ignoring the lowest 8 bits
  // 200 is very roughly the maximum number reached by any of the sensors
  double tmp_x = (magn[0] + 200) * (256/(double)(2*200));
  if (tmp_x > 255) {
	tmp_x = 255;
  }
  if (tmp_x < 0) {
	tmp_x = 0;
  }
  double tmp_y = (magn[1] + 200) * (256/(double)(2*200));
  if (tmp_y > 255) {
	tmp_y = 255;
  }
  if (tmp_y < 0) {
	tmp_y = 0;
  }
  double tmp_z = (magn[2] + 200) * (256/(double)(2*200));
  if (tmp_z > 255) {
	tmp_z = 255;
  }
  if (tmp_z < 0) {
	tmp_z = 0;
  }

  raw8bit[6] = (quint8)tmp_x;
  raw8bit[7] = (quint8)tmp_y;
  raw8bit[8] = (quint8)tmp_z;

  // Push data when we have new data from both the IMU and the PNI
  if (imuRead && pniRead) {
	imuRead = false;
	pniRead = false;
	imu->pushSensorData(raw8bit, ins);
  }

  pniData.clear();
}



Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOvero)

