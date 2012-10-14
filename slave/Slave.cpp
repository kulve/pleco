/*
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Slave.h"
#include "Transmitter.h"
#include "VideoSender.h"

#include <QCoreApplication>
#include <QPluginLoader>

#include <stdlib.h>                     /* getenv */

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

Slave::Slave(int &argc, char **argv):
  QCoreApplication(argc, argv), transmitter(NULL), stats(NULL),
  vs(NULL), status(0), hardware(NULL), mcuFD(-1), mcuPortName("/dev/ttyACM0"),
  mcuPort(), mcuData()
{
  stats = new QList<int>;

  QObject::connect(&mcuPort, SIGNAL(mcuReadyRead()),
				   this, SLOT(mcuReadPengingData()));
}



Slave::~Slave()
{ 
  // Close serial port to MCU
  if (mcuFD >=0) {
	mcuPort.close();
	close(mcuFD);
	mcuFD = -1;
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

  // Delete hardware
  if (hardware) {
	delete hardware;
	hardware = NULL;
  }
}



bool Slave::init(void)
{
  // Check on which hardware we are running based on the info in /proc/cpuinfo.
  // Defaulting to Generic X86
  QString hardwarePlugin("plugins/GenericX86/libgeneric_x86.so");
  QFile cpuinfo("/proc/cpuinfo");
  if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
	qDebug() << "Reading cpuinfo";
	QByteArray content = cpuinfo.readAll();

	// Check for supported hardwares
	if (content.contains("Gumstix Overo")) {
	  qDebug() << "Detected Gumstix Overo";
	  hardwarePlugin = "plugins/GumstixOvero/libgumstix_overo.so";
	} else if (content.contains("BCM2708")) {
	  qDebug() << "Detected Broadcom based Raspberry Pi";
	  hardwarePlugin = "plugins/GumstixOvero/libraspberry_pi.so";
	}

	cpuinfo.close();
  }


  qDebug() << "Loading hardware plugin:" << hardwarePlugin;
  // FIXME: add proper application specific plugin installation prefix.
  // For now we just seach so that the plugins in source code directories work.
  // FIXME: these doesn't affect pluginLoader?
  this->addLibraryPath("./plugins/GenericX86");
  this->addLibraryPath("./plugins/GumstixOvero");
  this->addLibraryPath("./plugins/RaspberryPi");

  QPluginLoader pluginLoader(hardwarePlugin);
  QObject *plugin = pluginLoader.instance();

  if (plugin) {
	hardware = qobject_cast<Hardware*>(plugin);
	if (!hardware) {
	  qCritical("Failed cast Hardware");
	}
  } else {
	qCritical("Failed to load plugin: %s", pluginLoader.errorString().toUtf8().data());
	qCritical("Search path: %s", this->libraryPaths().join(",").toUtf8().data());
  }

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
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));

  transmitter->initSocket();

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);

  // FIXME: add new stats gathering without executing an external script

  // Create and enable sending video
  if (vs) {
	delete vs;
  }
  vs = new VideoSender(hardware);

  QObject::connect(vs, SIGNAL(media(QByteArray*)), transmitter, SLOT(sendMedia(QByteArray*)));

}



bool Slave::setupMCU(void)
{
  if (!mcuOpenDevice()) {
	qCritical("Failed to open and setup MCU serial port");
	return false;
  }

  return true;
}



void Slave::mcuReadPendingData(void)
{
  while (mcuPort.bytesAvailable() > 0) {

	// FIXME: use ring buffer?
	mcuData.append(mcuPort.readAll());

	//qDebug() << "in" << __FUNCTION__ << ", data size: " << data.size();
  }

  mcuParseData();
}



bool Slave::mcuOpenDevice(void)
{
  struct termios newtio;

  // QFile doesn't support reading UNIX device nodes using readyRead()
  // signal, so we trick around that using TCP socket class.  We'll
  // set up the file descriptor without Qt and then pass the properly
  // set up file descriptor to QTcpSocket for handling the incoming
  // data.

  // Open device
  mcuFD = open(mcuPortName.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (mcuFD < 0) {
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
  tcflush(mcuFD, TCIFLUSH);
  tcsetattr(mcuFD, TCSANOW, &newtio);

  // Set the file descriptor for our TCP socket class
  mcuPort.setSocketDescriptor(mcuFD);
  mcuPort.setReadBufferSize(16);

  return true;
}



void Slave::mcuParseData(void)
{
  // TODO: We don't read back anything yet
  mcuData.clear();
  return;
}



bool Slave::mcuWriteData(char *msg)
{
  mcuPort.write(msg);
  return true;
}



bool Slave::mcuWriteData(QByteArray &msg)
{
  mcuPort.write(msg);
  return true;
}



// FIXME: this uses now control board's led interface. Should be more generic gpio interface
bool Slave::mcuGPIOSet(quint16 gpio, quint16 enable)
{
  QByteArray msg = "l";

  (void)gpio;

  if (enable) {
	msg += "1";
  } else {
	msg += "0";
  }

  msg += "\n";

  return mcuWriteData(msg);
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

  // Push slave status bits
  stats->push_back(status);

  // Send the latest stats
  sendStats();
}



// FIXME: use qint16?
void Slave::updateValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << type << ", value:" << value;

  switch (type) {
  case MSG_SUBTYPE_ENABLE_LED:
	mcuGPIOSet(1, value);
	break;
  case MSG_SUBTYPE_ENABLE_VIDEO:
	vs->enableSending(value?true:false);
	if (value) {
	  status ^= STATUS_VIDEO_ENABLED;
	} else {
	  status &= ~STATUS_VIDEO_ENABLED;
	}
	break;
  case MSG_SUBTYPE_VIDEO_SOURCE:
	vs->setVideoSource(value);
	break;
  default:
	qWarning("%s: Unknown type: %d", __FUNCTION__, type);
  }
}
