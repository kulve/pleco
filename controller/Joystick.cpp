/*
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#include <sys/stat.h>        // open
#include <fcntl.h>           // open
#include <unistd.h>          // close
#include <string.h>          // strerror
#include <errno.h>           // errno
#include <linux/joystick.h>

#include "Joystick.h"

/*
 * Constructor for the Joystick
 */
Joystick::Joystick(void):
  inputDevice(), enabled(false), fd(0)
{
  // Nothing here
}



/*
 * Destructor for the Joystick
 */
Joystick::~Joystick()
{
  if (enabled) {
	close(fd);
	enabled = false;
  }
}



/*
 * Init Joystick. Returns false on failure
 */
bool Joystick::init(QString inputDevicePath)
{

  // Open the input device using traditional open()
  fd = open(inputDevicePath.toAscii().data(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    qCritical("Failed to open Control Board device (%s): %s", inputDevicePath.toAscii().data(), strerror(errno));
    return false;
  }

  // Pass the fd to QLocalSocket
  inputDevice.setSocketDescriptor(fd);
  inputDevice.setReadBufferSize(sizeof(struct js_event));

  QObject::connect(&inputDevice, SIGNAL(readyRead()),
                   this, SLOT(readPendingInputData()));

  enabled = true;
  return true;
}



/*
 * Read data from the input device
 */
void Joystick::readPendingInputData(void)
{
  struct js_event *js;
  QByteArray buf;

  //qDebug() << "in" << __FUNCTION__ << ", data size: " << inputDevice.size();

  buf = inputDevice.readAll();
  if (buf.length() < (int)(sizeof(struct js_event))) {
	qWarning("Too few bytes read: %d", buf.length());
	return;
  }

  js = (struct js_event *)buf.data();

  //qDebug() << "Joystick: type: " << js->type << "number: " << js->number << "value: " << js->value;

  // Handle button press
  if (js->type == 1) {
	emit(buttonChanged(js->number, static_cast<quint16>(js->value)));
	return;
  }

  // Handle axis movement
  if (js->type == 2) {
	quint16 value = (quint16)(js->value / 256.0 + 128);
	emit(axisChanged(js->number, value));
	return;
  }
}
