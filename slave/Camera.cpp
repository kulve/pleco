/*
 * Copyright 2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#include "Camera.h"

#include <QObject>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>          // errno
#include <string.h>         // strerror
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/videodev2.h>

Camera::Camera(void):
  fd(-1), auto_focus(true)
{
}



Camera::~Camera()
{
  if (fd > -1) {
	close(fd);
  }
}



bool Camera::init(void)
{
  // Open camera device
  fd = open("/dev/video0", O_RDWR);
  if (fd < 0) {
    qCritical("Failed to open V4L2 device (%s): %s", "/dev/video0", strerror(errno));
	return false;
  }

  return true;
}



bool Camera::setBrightness(quint8 value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  if (fd < 0) {
	qWarning("Camera not initialised.");
	return false;
  }

  memset(&control, 0, sizeof (control));
  memset(&query, 0, sizeof (query));

  query.id = V4L2_CID_BRIGHTNESS;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
    qCritical("Failed to query brightness: %s", strerror(errno));
	return false;
  }

  control.id = V4L2_CID_BRIGHTNESS;
  // Scale brightness according to value (in %) to between min and max
  control.value = (int)((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
    qCritical("Failed to set brightness values: %s", strerror(errno));
	return false;
  }

  qDebug() << "in" << __FUNCTION__ << ", brightness set to" << control.value;

  return true;
}



bool Camera::setZoom(quint8 value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  if (fd < 0) {
	qWarning("Camera not initialised.");
	return false;
  }

  memset(&control, 0, sizeof (control));
  memset(&query, 0, sizeof (query));

  query.id = V4L2_CID_ZOOM_ABSOLUTE;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
    qCritical("Failed to query zoom: %s", strerror(errno));
	return false;
  }

  control.id = V4L2_CID_ZOOM_ABSOLUTE;
  // Scale zoom according to value (in %) to between min and max
  control.value = (int)((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
    qCritical("Failed to set zoom values: %s", strerror(errno));
	return false;
  }

  qDebug() << "in" << __FUNCTION__ << ", zoom set to" << control.value;

  return true;
}



bool Camera::setFocus(quint8 value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  bool new_auto_focus = (value == 0);

  if (fd < 0) {
	qWarning("Camera not initialised.");
	return false;
  }

  memset(&control, 0, sizeof (control));
  memset(&query, 0, sizeof (query));

  // Enable or disable auto focus
  if (auto_focus != new_auto_focus) {
	control.id = V4L2_CID_FOCUS_AUTO;
	control.value = new_auto_focus ? 1 : 0;

	if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
	  qCritical("Failed to set auto focus: %s", strerror(errno));
	  return false;
	}

	auto_focus = new_auto_focus;

	qDebug() << "in" << __FUNCTION__ << ", focus set to" << control.value;
  }

  // Nothing more to do if auto focus enabled
  if (auto_focus) {
	return true;
  }

  // Set manual focus, 1-100%
  query.id = V4L2_CID_FOCUS_ABSOLUTE;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
	qCritical("Failed to query focus: %s", strerror(errno));
	return false;
  }

  control.id = V4L2_CID_FOCUS_ABSOLUTE;
  // Scale focus according to value (in %) to between min and max
  control.value = (int)((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
	qCritical("Failed to set focus values: %s", strerror(errno));
	return false;
  }

  qDebug() << "in" << __FUNCTION__ << ", focus set to" << control.value;

  return true;
}
