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


// millidegrees/second
#define PLECO_DEG_PER_TICK 0.977

// Length of the message from the SparkFun 6DoF module in binary mode
#define PLECO_6DOF_DATA_LEN 16


GumstixOvero::GumstixOvero(void) :
  imu(NULL), fd(-1), serialPortName("/dev/ttyO0"), serialPort(), data(),
  pni11096("/dev/pni11096"), pniTimer(NULL), pniRead(false)
{

  QObject::connect(&serialPort, SIGNAL(readyRead()),
				   this, SLOT(readPendingData()));

  magn[0] = magn[1] = magn[2] = 0;

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
	
	// Disable 6DoF
	if (fd >=0) {
	  serialPort.close();
	  close(fd);
	  fd = -1;
	}
	// Disable PNI
	if (pniTimer) {
	  delete pniTimer;
	}
	pni11096.close();
	return true;
  }

  // Enable IMU
  if (!openSerialDevice(serialPortName)) {
	qCritical("Failed to open and setup serial port");
	return false;
  }

  // Open PNI11096 magnetometer device
  if (!pni11096.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
	qCritical("Failed to open PNI11096 device");
	return false;
  }

  // Start reading PNI data X times a second
  if (pniTimer) {
	delete pniTimer;
  }
  pniTimer = new QTimer(this);
  QObject::connect(pniTimer, SIGNAL(timeout()), this, SLOT(readPNI()));
  pniTimer->start(10); // 100Hz

  return true;
}



void GumstixOvero::readPendingData(void)
{
  while (serialPort.bytesAvailable() > 0) {

	// FIXME: use ring buffer?
	data.append(serialPort.readAll());
	
	//qDebug() << "in" << __FUNCTION__ << ", data size: " << data.size();
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
  serialPort.setReadBufferSize(16);

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
	  ins[i] = (data[data_i+1] + (data[data_i] << 8));
	  raw8bit[i] = (quint8)((quint32)(ins[i]) >> 2); /* ignore 2 bits of the 10bit data */
	  ins[i] -= 512; // 10bit values to signed
	}
	
	// Just a sanity check
	if (!data.at(msgStart + PLECO_6DOF_DATA_LEN - 1) == 'Z') {
	  qWarning("Invalid data, skipping: %s", data.toHex().data());
	  return;
	}

    /* Convert gyroscope values to degrees per second. Absolute
	   scale of magnetometer and accelerometer doesn't matter */
    ins[3] *= PLECO_DEG_PER_TICK;
    ins[4] *= PLECO_DEG_PER_TICK;
    ins[5] *= PLECO_DEG_PER_TICK;

	// Insert last read PNI 11096 magnetometer data
	ins[6] = magn[0]; // PNI x
	ins[7] = magn[1]; // PNI y
	ins[8] = magn[2]; // PNI z

	// Convert 16bit values to unsigned 8bit by ignoring the lowest 8 bits
	raw8bit[6] = (quint8)(((quint16)(magn[0] + 32767)) >> 8);
	raw8bit[7] = (quint8)(((quint16)(magn[1] + 32767)) >> 8);
	raw8bit[8] = (quint8)(((quint16)(magn[2] + 32767)) >> 8);

	if (imu && pniRead) {
	  imu->pushSensorData(raw8bit, ins);
	}

	// FIXME: use ring buffer?
	// Remove the handled message and the possible garbage before it from the data
	data.remove(0, msgStart + PLECO_6DOF_DATA_LEN); 
}



void GumstixOvero::readPNI(void)
{
  qint16 data[6];
  int result;

  result = pni11096.read((char*)data, 12); // 16bit signed for x, y and z axes with labels

  if (result == -1) {
	qCritical("Failed to read PNI device: %d", pni11096.error());
	// FIXME: some how indicate IMU failure to Controller?
	return;
  }

  if (result < 12) {
	qWarning("Failed to read full data from PNI device, ignoring");
	return;
  }

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
  magn[2] = -1 * (qint16)(data[5]);

  // Flag that we have PNI data;
  pniRead = true;
}



Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOvero)

