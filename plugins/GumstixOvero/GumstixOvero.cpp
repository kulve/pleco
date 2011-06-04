#include "GumstixOvero.h"

#include <QtPlugin>

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>          // errno
#include <string.h>         // strerror


// millidegrees/second
#define PLECO_DEG_PER_TICK 0.977

// Length of the message from the SparkFun 6DoF module in binary mode
#define PLECO_6DOF_DATA_LEN 16


GumstixOvero::GumstixOvero(void) :
  imu(NULL), fd(-1), serialPortName("/dev/ttyO0"), serialPort(), data()
{

  QObject::connect(&serialPort, SIGNAL(readyRead()),
				   this, SLOT(readPendingData()));


}



GumstixOvero::~GumstixOvero(void)
{
  
  // Close serial port
  if (fd >= 0) {
	serialPort.close();
	close(fd);
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

  // We are using SparkFun 6DoF IMU + PNI11096 magnetometer and they
  // don't need any initialisation
  return true;
}



bool GumstixOvero::enableIMU(bool enable)
{

  // Disable IMU
  if (!enable) {
	if (fd >=0) {
	  serialPort.close();
	  close(fd);
	  fd = -1;
	}
	// FIXME: disable PNI
	return true;
  }

  // Enable IMU
  if (!openSerialDevice(serialPortName)) {
	qCritical("Failed to open and setup serial port");
	return false;
  }


  // FIXME: start reading PNI from /dev/XXX

  return true;
}



void GumstixOvero::readPendingData(void)
{
  while (serialPort.bytesAvailable() > 0) {

	// FIXME: use ring buffer?
	data.append(serialPort.readAll());
	
	qDebug() << "in" << __FUNCTION__ << ", data size: " << data.size();
  }

  parseData();
}



bool GumstixOvero::openSerialDevice(QString device)
{
  struct termios newtio;
  
  // QFile doesn't support reading UNIX device nodes, so we trick
  // around that using TCP socket class.  We'll set up the file
  // descriptor without Qt and then pass the properly set up file
  // descriptor to QTcpSocket for handling the incoming data.
  
  // Open device
  fd = open(device.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK); 
  if (fd < 0) { 
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
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &newtio);
  
  // Set the file descriptor for our TCP socket class
  serialPort.setSocketDescriptor(fd);

  return true;
}



void GumstixOvero::parseData(void)
{
	int msgStart = -1;
	double ins[9];
	quint8 raw8bit[9];

	if (data.size() < PLECO_6DOF_DATA_LEN) {
	  // Not enough data
	  return;
	}

	// Check if the data buffer has a valid message (it should as we have enough data)
	for (int i = 0; i < data.size() - PLECO_6DOF_DATA_LEN; ++i) {
	  if (data.at(i) == 'A' && data.at(i + PLECO_6DOF_DATA_LEN - 1) == 'Z') {
		msgStart = i;
		break;
	  }
	}

	// No valid data even though we have enough data for one message
	if (msgStart == -1) {

	  // If we have over twice the message length of data without a valid message, clear the extra from the start.
	  if (data.size() > 2 * PLECO_6DOF_DATA_LEN) {
		data.remove(0, data.size() - PLECO_6DOF_DATA_LEN);
	  }
	  return;
	}

	// We have a valid message from the IMU unit starting at msgStart

	// Fill the 16bit 6DoF values after 'A' and 16bit count
	for (int i = 0; i < 6; i++) { 
	  int data_i = msgStart + i*2 + 3;
	  ins[i] = (data[data_i+1] + (data[data_i]  << 8));
	  raw8bit[i] = (quint8)((quint32)(ins[i]) >> 2); /* ignore 2 bits of the 10bit data */
	}
	
	// FIXME: valid IMU, not yet magnetometer data
	raw8bit[6] = ins[6] = 0; // PNI x
	raw8bit[7] = ins[7] = 0; // PNI y
	raw8bit[8] = ins[8] = 0; // PNI z

	// Just a sanity check
	if (!data.at(msgStart + PLECO_6DOF_DATA_LEN - 1) == 'Z') {
	  qWarning("Invalid data, skipping:");
	  qWarning(data.toHex().data());
	  return;
	}

    /* Convert gyroscope values to millidegrees per second. Absolute
	   scale of magnetometer and accelerometer doesn't matter */
    ins[3] *= PLECO_DEG_PER_TICK;
    ins[4] *= PLECO_DEG_PER_TICK;
    ins[5] *= PLECO_DEG_PER_TICK;

	if (imu) {
	  imu->pushSensorData(raw8bit, ins);
	}

	// FIXME: use ring buffer?
	// Remove the handled message and the possible garbage before it from the data
	data.remove(0, msgStart + PLECO_6DOF_DATA_LEN); 
}



Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOvero)
