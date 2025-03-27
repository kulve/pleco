/*
 * Copyright 2015-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Camera.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
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
  const char *camera = "/dev/video0";
  char* env_camera = std::getenv("PLECO_SLAVE_CAMERA");
  if (env_camera != nullptr) {
    camera = env_camera;
  }
  fd = open(camera, O_RDWR);
  if (fd < 0) {
    std::cerr << "Failed to open V4L2 device (" << camera << "): "
              << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

bool Camera::setBrightness(std::uint8_t value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  if (fd < 0) {
    std::cerr << "Camera not initialised." << std::endl;
    return false;
  }

  memset(&control, 0, sizeof(control));
  memset(&query, 0, sizeof(query));

  query.id = V4L2_CID_BRIGHTNESS;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
    std::cerr << "Failed to query brightness: " << strerror(errno) << std::endl;
    return false;
  }

  control.id = V4L2_CID_BRIGHTNESS;
  // Scale brightness according to value (in %) to between min and max
  control.value = static_cast<int>((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
    std::cerr << "Failed to set brightness values: " << strerror(errno) << std::endl;
    return false;
  }

  std::cout << "in " << __FUNCTION__ << ", brightness set to " << control.value << std::endl;

  return true;
}

bool Camera::setZoom(std::uint8_t value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  if (fd < 0) {
    std::cerr << "Camera not initialised." << std::endl;
    return false;
  }

  memset(&control, 0, sizeof(control));
  memset(&query, 0, sizeof(query));

  query.id = V4L2_CID_ZOOM_ABSOLUTE;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
    std::cerr << "Failed to query zoom: " << strerror(errno) << std::endl;
    return false;
  }

  control.id = V4L2_CID_ZOOM_ABSOLUTE;
  // Scale zoom according to value (in %) to between min and max
  control.value = static_cast<int>((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
    std::cerr << "Failed to set zoom values: " << strerror(errno) << std::endl;
    return false;
  }

  std::cout << "in " << __FUNCTION__ << ", zoom set to " << control.value << std::endl;

  return true;
}

bool Camera::setFocus(std::uint8_t value)
{
  struct v4l2_control control;
  struct v4l2_queryctrl query;

  bool new_auto_focus = (value == 0);

  if (fd < 0) {
    std::cerr << "Camera not initialised." << std::endl;
    return false;
  }

  memset(&control, 0, sizeof(control));
  memset(&query, 0, sizeof(query));

  // Enable or disable auto focus
  if (auto_focus != new_auto_focus) {
    control.id = V4L2_CID_FOCUS_AUTO;
    control.value = new_auto_focus ? 1 : 0;

    if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
      std::cerr << "Failed to set auto focus: " << strerror(errno) << std::endl;
      return false;
    }

    auto_focus = new_auto_focus;

    std::cout << "in " << __FUNCTION__ << ", focus set to " << control.value << std::endl;
  }

  // Nothing more to do if auto focus enabled
  if (auto_focus) {
    return true;
  }

  // Set manual focus, 1-100%
  query.id = V4L2_CID_FOCUS_ABSOLUTE;
  if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == -1) {
    std::cerr << "Failed to query focus: " << strerror(errno) << std::endl;
    return false;
  }

  control.id = V4L2_CID_FOCUS_ABSOLUTE;
  // Scale focus according to value (in %) to between min and max
  control.value = static_cast<int>((((query.maximum - query.minimum) / 100.0) * value) + query.minimum);

  if (ioctl(fd, VIDIOC_S_CTRL, &control) == -1) {
    std::cerr << "Failed to set focus values: " << strerror(errno) << std::endl;
    return false;
  }

  std::cout << "in " << __FUNCTION__ << ", focus set to " << control.value << std::endl;

  return true;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
