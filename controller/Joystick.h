/*
 * Copyright 2014-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Event.h"

#include <string>
#include <functional>
#include <cstdint>
#include <array>
#include <linux/joystick.h>

// Forward declarations
class Timer;

// Joystick constants
constexpr const char* JOYSTICK_INPUT_DEV = "/dev/input/js0";
constexpr std::size_t JOYSTICK_NAME_LEN = 128;

// Joystick axis definitions
constexpr int JOYSTICK_AXIS_STEER = 0;
constexpr int JOYSTICK_AXIS_THROTTLE = 1;

// Joystick button definitions
constexpr int JOYSTICK_BTN_REVERSE = 0;

class Joystick
{
 public:
  // Constructor takes event loop reference
  Joystick(EventLoop& eventLoop);
  ~Joystick();

  bool init(const std::string& inputDevicePath = JOYSTICK_INPUT_DEV);

  // Callbacks to replace Qt signals
  using AxisCallback = std::function<void(int axis, std::uint16_t value)>;
  using ButtonCallback = std::function<void(int button, std::uint16_t value)>;

  // Methods to set callbacks
  void setAxisCallback(AxisCallback callback);
  void setButtonCallback(ButtonCallback callback);

 private:
  // Read input data from the joystick
  void readPendingInputData();

  // ASIO file descriptor wrapper for joystick
  asio::posix::stream_descriptor joystickDesc;

  // State tracking
  bool enabled;
  int fd;

  // Information about axes and buttons
  std::array<char*, ABS_MAX + 1> axis_names;
  std::array<char*, KEY_MAX - BTN_MISC + 1> button_names;
  char name[JOYSTICK_NAME_LEN];
  std::uint8_t joystick;

  // Callbacks
  AxisCallback axisCallback;
  ButtonCallback buttonCallback;

  // Buffer for reading events
  js_event event;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/