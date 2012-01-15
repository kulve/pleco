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

Slave::Slave(int &argc, char **argv):
  QCoreApplication(argc, argv), transmitter(NULL), stats(NULL),
  vs(NULL), status(0), hardware(NULL)
{
  stats = new QList<int>;
}



Slave::~Slave()
{ 
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

	// Check for Gumstix Overo
	if (content.contains("Gumstix Overo")) {
	  qDebug() << "Detected Gumstix Overo";
	  hardwarePlugin = "plugins/GumstixOvero/libgumstix_overo.so";
	}

	cpuinfo.close();
  }


  qDebug() << "Loading hardware plugin:" << hardwarePlugin;
  // FIXME: add proper application specific plugin installation prefix.
  // For now we just seach so that the plugins in source code directories work.
  // FIXME: these doesn't affect pluginLoader?
  this->addLibraryPath("./plugins/GenericX86");
  this->addLibraryPath("./plugins/GumstixOvero");

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
