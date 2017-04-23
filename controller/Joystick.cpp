/*
 * Copyright 2013 Tuomas Kulve, <tuomas@kulve.fi>
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

#define MAX_FEATURE_COUNT   32

struct joystick_st {
  QString name;
  qint16 remap[2][MAX_FEATURE_COUNT];
};

static joystick_st supported[] = {
  "generic default joystick without axis mapping",
  {{-1}},
  "COBRA M5",
  {{-1}}
};

/*
 * Constructor for the Joystick
 */
Joystick::Joystick(void):
  inputDevice(), enabled(false), fd(0), joystick(0)
{
  for (quint8 j = 1; j < sizeof(supported)/sizeof(supported[0]); ++j) {
    for (int t = 0; t < 2; ++t) {
      for (int f = 0; f < MAX_FEATURE_COUNT; ++f) {
        supported[j].remap[t][f] = -1;
      }
    }
  }

  // Remap for Cobra M5
  supported[1].remap[0][8] = JOYSTICK_BTN_REVERSE;
  supported[1].remap[1][3] = JOYSTICK_AXIS_STEER;
  supported[1].remap[1][2] = JOYSTICK_AXIS_THROTTLE;

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
  fd = open(inputDevicePath.toUtf8(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    qCritical() << "Failed to open joystick device:" << inputDevicePath << strerror(errno);
    return false;
  }

	if (ioctl(fd, JSIOCGNAME(JOYSTICK_NAME_LEN), name) == -1) {
    qCritical() << "Failed to get joystick name:" << strerror(errno);
    return false;
  }
  qDebug() << "Detected joystick" << name;

  QString jstr(name);
  for (quint8 j = 1; j < sizeof(supported)/sizeof(supported[0]); ++j) {
    if (jstr.contains(supported[j].name)) {
      joystick = j;
      qDebug() << "Found joystick mappings for" << supported[j].name;
      break;
    }
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

  //qDebug() << "Joystick: index: " << joystick << "type: " << js->type << "number: " << js->number << "value: " << js->value;

  if (js->type != 1 && js->type != 2) {
    //qDebug() << "Joystick: Ignoring unhandled js->type:" << js->type;
    return;
  }

  int ab_number = js->number;
  if (ab_number >= MAX_FEATURE_COUNT) {
    qWarning("Axis/button number to high, ignoring: %d", ab_number);
    return;
  }

  if (joystick > 0) {
    int tmp = supported[joystick].remap[js->type - 1][ab_number];
    if (tmp == -1) {
      //qDebug() << "Joystick: Ignoring unmapped axis/button (" << js->type << "):" << js->number;
      return;
    }

    ab_number = tmp;
  }

  // Handle button press
  if (js->type == 1) {
    emit(buttonChanged(ab_number, static_cast<quint16>(js->value)));
    return;
  }

  // Handle axis movement
  if (js->type == 2) {
    quint16 value = (quint16)(js->value / 256.0 + 128);
    emit(axisChanged(ab_number, value));
    return;
  }
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
