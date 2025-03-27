/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Event.h"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

// Control board PWM channel definitions
namespace CB_PWM {
  constexpr std::uint8_t PWM1 = 1;
  constexpr std::uint8_t PWM2 = 2;
  constexpr std::uint8_t PWM3 = 3;
  constexpr std::uint8_t PWM4 = 4;
  constexpr std::uint8_t PWM5 = 5;
  constexpr std::uint8_t PWM6 = 6;
  constexpr std::uint8_t PWM7 = 7;
  constexpr std::uint8_t PWM8 = 8;

  // Device specific PWM mappings
  constexpr std::uint8_t SPEED       = PWM1;
  constexpr std::uint8_t TURN        = PWM2;
  constexpr std::uint8_t TURN2       = PWM7;
  constexpr std::uint8_t CAMERA_X    = PWM3;
  constexpr std::uint8_t CAMERA_Y    = PWM4;
  constexpr std::uint8_t SPEED_LEFT  = PWM5;
  constexpr std::uint8_t SPEED_RIGHT = PWM6;
}

// Control board GPIO definitions
namespace CB_GPIO {
  constexpr std::uint16_t DIRECTION_LEFT       = 2;
  constexpr std::uint16_t DIRECTION_RIGHT      = 3;
  constexpr std::uint16_t SPEED_ENABLE_LEFT    = 4;
  constexpr std::uint16_t SPEED_ENABLE_RIGHT   = 5;
  constexpr std::uint16_t LED1                 = 0; // HACK
  constexpr std::uint16_t HEAD_LIGHTS          = 5;
  constexpr std::uint16_t REAR_LIGHTS          = 1;
}

// Forward declaration of our Timer implementation
class Timer;

class ControlBoard
{
 public:
  ControlBoard(EventLoop& eventLoop, const std::string& serialDevice);
  ~ControlBoard();

  bool init(void);
  void setPWMFreq(std::uint32_t freq);
  void setPWMDuty(std::uint8_t pwm, std::uint16_t duty);
  void stopPWM(std::uint8_t pwm);
  void setGPIO(std::uint16_t gpio, std::uint16_t enable);
  void sendPing(void);

  // Callback types
  using DebugCallback = std::function<void(const std::string&)>;
  using ValueCallback = std::function<void(std::uint16_t)>;

  // Set callbacks
  void setDebugCallback(DebugCallback callback);
  void setTemperatureCallback(ValueCallback callback);
  void setDistanceCallback(ValueCallback callback);
  void setCurrentCallback(ValueCallback callback);
  void setVoltageCallback(ValueCallback callback);

 private:
  void readPendingSerialData(void);
  void portError(int error);
  void portDisconnected(void);
  void reopenSerialDevice(void);
  void parseSerialData(void);
  bool openSerialDevice(void);
  void closeSerialDevice(void);
  void writeSerialData(const std::string& msg);

  // Reference to event loop for timers and async operations
  EventLoop& eventLoop;

  // Serial port using ASIO
  asio::posix::stream_descriptor serial_port;

  std::string serialDevice;
  std::vector<std::uint8_t> serialData;
  bool enabled;

  // Timers using ASIO
  std::shared_ptr<Timer> reopenTimer;
  std::shared_ptr<Timer> wdgTimer;

  // Callbacks
  DebugCallback debugCallback;
  ValueCallback temperatureCallback;
  ValueCallback distanceCallback;
  ValueCallback currentCallback;
  ValueCallback voltageCallback;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/