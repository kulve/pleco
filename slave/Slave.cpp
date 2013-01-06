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
  vs(NULL), status(0), hardware(NULL), cb(NULL),
  oldSpeed(0), oldTurn(0)
{
  stats = new QList<int>;
}



Slave::~Slave()
{

  // Delete the control board, if any
  if (cb) {
	delete cb;
	cb = NULL;
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

  // FIXME: get serial device path from hardware plugin?
  cb = new ControlBoard("/dev/ttyACM0");
  if (!cb->init()) {
	qCritical("Failed to initialize ControlBoard");
	// CHECKME: to return false or not to return false (and do clean up)?
  } else {
	// Set ControlBoard frequency to 50Hz to match standard servos
	cb->setPWMFreq(50);
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
  QObject::connect(transmitter, SIGNAL(connectionStatusChanged(int)), this, SLOT(updateConnectionStatus(int)));

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

  QObject::connect(cb, SIGNAL(debug(QString*)), transmitter, SLOT(sendDebug(QString*)));
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



void Slave::updateValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << Message::getSubTypeStr(type) << ", value:" << value;

  switch (type) {
  case MSG_SUBTYPE_ENABLE_LED:
	cb->setGPIO(CB_GPIO_LED1, value);
	break;
  case MSG_SUBTYPE_ENABLE_VIDEO:
	parseSendVideo(value);
	break;
  case MSG_SUBTYPE_VIDEO_SOURCE:
	vs->setVideoSource(value);
	break;
  case MSG_SUBTYPE_CAMERA_XY:
	parseCameraXY(value);
	break;
  case MSG_SUBTYPE_SPEED_TURN:
	parseSpeedTurn(value);
	break;
  default:
	qWarning("%s: Unknown type: %s", __FUNCTION__, Message::getSubTypeStr(type).toAscii().data());
  }
}


void Slave::updateConnectionStatus(int status)
{
  qDebug() << "in" << __FUNCTION__ << ", status:" << status;

  if (status == CONNECTION_STATUS_LOST) {

	// Stop moving if lost connection to the controller
	if (oldSpeed != 0) {
	  cb->setPWMDuty(CB_PWM_SPEED, 750);
	  cb->stopPWM(CB_PWM_SPEED);
	  qDebug() << "in" << __FUNCTION__ << ", Speed PWM:" << 750;
	  oldSpeed = 0;
	}

	if (oldTurn != 0) {
	  cb->setPWMDuty(CB_PWM_TURN, 750);
	  cb->stopPWM(CB_PWM_TURN);
	  qDebug() << "in" << __FUNCTION__ << ", Turn PWM:" << 750;
	  oldTurn = 0;
	}

	// Stop sending video
	parseSendVideo(0);
  }

  // FIXME: if connection restored (or just ok for the first time), send status to controller?
}



void Slave::parseSendVideo(quint16 value)
{
  vs->enableSending(value ? true : false);
  if (value) {
	status ^= STATUS_VIDEO_ENABLED;
  } else {
	status &= ~STATUS_VIDEO_ENABLED;
  }
}



void Slave::parseCameraXY(quint16 value)
{
  quint16 x, y;
  static quint16 oldx = 0, oldy = 0;

  // Value is a 16 bit containing 2x 8bit values that are doubled percentages
  x = (value >> 8);
  y = (value & 0x00ff);

  // Control board expects percentages to be x100 integers
  // Servos expect 5-10% duty cycle for the 1-2ms pulses
  x = static_cast<quint16>(x * (5 / 2.0)) + 500;
  y = static_cast<quint16>(y * (5 / 2.0)) + 500;

  // Update servo positions only if value has changed
  if (x != oldx) {
	cb->setPWMDuty(CB_PWM_CAMERA_X, x);
	qDebug() << "in" << __FUNCTION__ << ", Camera X PWM:" << x;
	oldx = x;
  }

  if (y != oldy) {
	cb->setPWMDuty(CB_PWM_CAMERA_Y, y);
	qDebug() << "in" << __FUNCTION__ << ", Camera Y PWM:" << y;
	oldy = y;
  }
}



void Slave::parseSpeedTurn(quint16 value)
{
  quint16 speed, turn;

  // Value is a 16 bit containing 2x 8bit values that are shifted by 100
  speed = (value >> 8);
  turn = (value & 0x00ff);

  // Control board expects percentages to be x100 integers
  // Servos expect 5-10% duty cycle for the 1-2ms pulses
  speed = static_cast<quint16>(speed * (5 / 2.0)) + 500;
  turn = static_cast<quint16>(turn * (5 / 2.0)) + 500;

  // Update servo/ESC positions only if value has changed
  if (speed != oldSpeed) {
	cb->setPWMDuty(CB_PWM_SPEED, speed);
	qDebug() << "in" << __FUNCTION__ << ", Speed PWM:" << speed;
	oldSpeed = speed;
  }

  if (turn != oldTurn) {
	cb->setPWMDuty(CB_PWM_TURN, turn);
	qDebug() << "in" << __FUNCTION__ << ", Turn PWM:" << turn;
	oldTurn = turn;
  }
}
