/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Controller.h"
#include "VideoReceiver.h"
#include "AudioReceiver.h"
#include "Joystick.h"
#include "Timer.h"

#include <iostream>
#include <cmath>
#include <algorithm>

// Limit the frequency of sending commands affecting PWM signals
#define THROTTLE_FREQ_CAMERA_XY  50
#define THROTTLE_FREQ_SPEED_TURN 50

// Limit the motor speed change
#define MOTOR_SPEED_GRACE_LIMIT  10

Controller::Controller(EventLoop& loop, int &argc, char **argv)
  : joystick(nullptr),
    transmitter(nullptr),
    vr(nullptr),
    ar(nullptr),
    padCameraXPosition(0),
    padCameraYPosition(0),
    cameraX(0),
    cameraY(0),
    motorSpeedTarget(0),
    motorSpeed(0),
    motorReverse(false),
    motorSpeedUpdateTimer(nullptr),
    motorTurn(0),
    cameraXYPending(false),
    speedTurnPending(false),
    speedTurnPendingSpeed(0),
    speedTurnPendingTurn(0),
    videoBufferPercent(0),
    videoQuality(50),
    cameraFocusPercent(50),
    cameraZoomPercent(50),
    motorHalfSpeed(false),
    calibrate(false),
    audioState(false),
    videoState(false),
    ledState(false),
    throttleTimerCameraXY(nullptr),
    throttleTimerSpeedTurn(nullptr),
    eventLoop(loop)
{
  // Initialize, but do not reference argc or argv
  (void)argc;
  (void)argv;
}

Controller::~Controller()
{
  // All smart pointers will clean up automatically
}

void Controller::createGUI()
{
  // Create joystick and set up callbacks
  joystick = std::make_unique<Joystick>(eventLoop);
  joystick->init();
  joystick->setButtonCallback([this](int button, std::uint16_t value) {
    buttonChanged(button, value);
  });
  joystick->setAxisCallback([this](int axis, std::uint16_t value) {
    axisChanged(axis, value);
  });

  // Create timer for camera updates
  const int freq = 50;
  auto cameraTimer = std::make_shared<Timer>(eventLoop);
  cameraTimer->start(1000/freq, [this]() {
    updateCameraPeriodically();
  }, true);

  // Create audio receiver
  ar = std::make_unique<AudioReceiver>();
}

void Controller::buttonChanged(int button, std::uint16_t value)
{
  std::cout << "Button changed: " << button << ", value: " << value << std::endl;

  switch(button) {
  case JOYSTICK_BTN_REVERSE:
    motorReverse = (value == 1);
    break;
  default:
    std::cout << "Joystick: Button unused: " << button << std::endl;
  }
}

void Controller::enableLed(bool enable)
{
  if (ledState == enable) {
    return;
  }
  ledState = enable;

  std::cout << "LED state changed: " << (ledState ? "enabled" : "disabled") << std::endl;

  if (!transmitter) return;

  if (ledState) {
    transmitter->sendValue(MessageSubtype::EnableLED, 1);
  } else {
    transmitter->sendValue(MessageSubtype::EnableLED, 0);
  }
}

void Controller::enableVideo(bool enable)
{
  if (videoState == enable) {
    return;
  }
  videoState = enable;

  std::cout << "Video state changed: " << (videoState ? "enabled" : "disabled") << std::endl;

  if (!transmitter) return;

  if (videoState) {
    transmitter->sendValue(MessageSubtype::EnableVideo, 1);
  } else {
    transmitter->sendValue(MessageSubtype::EnableVideo, 0);
  }
}

void Controller::enableAudio(bool enable)
{
  if (audioState == enable) {
    return;
  }
  audioState = enable;

  std::cout << "Audio state changed: " << (audioState ? "enabled" : "disabled") << std::endl;

  if (!transmitter) return;

  if (audioState) {
    transmitter->sendValue(MessageSubtype::EnableAudio, 1);
  } else {
    transmitter->sendValue(MessageSubtype::EnableAudio, 0);
  }
}

void Controller::sendVideoQuality()
{
  std::cout << "Setting video quality: " << videoQuality << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::VideoQuality, videoQuality);
}

void Controller::sendVideoSource(int source)
{
  std::cout << "Setting video source: " << source << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::VideoSource, static_cast<std::uint16_t>(source));
}

void Controller::sendCameraXYPending()
{
  if (cameraXYPending) {
    cameraXYPending = false;
    sendCameraXY();
  }
}

void Controller::sendCameraXY()
{
  if (!transmitter) return;

  // Send camera X and Y as percentages with 0.5 precision (i.e. doubled)
  std::uint8_t x = static_cast<std::uint8_t>(((cameraX + 90) / 180.0) * 100 * 2);
  std::uint8_t y = static_cast<std::uint8_t>(((cameraY + 90) / 180.0) * 100 * 2);

  std::uint16_t value = (x << 8) | y;

  if (!throttleTimerCameraXY) {
    throttleTimerCameraXY = std::make_shared<Timer>(eventLoop);
  }

  if (throttleTimerCameraXY->isActive()) {
    cameraXYPending = true;
    return;
  } else {
    throttleTimerCameraXY->start(1000/THROTTLE_FREQ_CAMERA_XY, [this]() {
      sendCameraXYPending();
    });
  }

  transmitter->sendValue(MessageSubtype::CameraXY, value);
}

void Controller::sendSpeedTurnPending()
{
  if (speedTurnPending) {
    speedTurnPending = false;
    sendSpeedTurn(speedTurnPendingSpeed, speedTurnPendingTurn);
  }
}

void Controller::sendSpeedTurn(int speed, int turn)
{
  if (!transmitter) return;

  motorSpeed = speed;
  motorTurn = turn;

  if (!throttleTimerSpeedTurn) {
    throttleTimerSpeedTurn = std::make_shared<Timer>(eventLoop);
  }

  if (throttleTimerSpeedTurn->isActive()) {
    speedTurnPending = true;
    speedTurnPendingSpeed = speed;
    speedTurnPendingTurn = turn;
    return;
  } else {
    throttleTimerSpeedTurn->start(1000/THROTTLE_FREQ_SPEED_TURN, [this]() {
      sendSpeedTurnPending();
    });
  }

  if (motorHalfSpeed) {
    speed /= 2;
  }

  // Send speed and turn as percentages shifted to 0 - 200
  std::uint8_t x = static_cast<std::uint8_t>(speed + 100);
  std::uint8_t y = static_cast<std::uint8_t>(turn + 100);
  std::uint16_t value = (x << 8) | y;

  transmitter->sendValue(MessageSubtype::SpeedTurn, value);
}

void Controller::sendCameraZoom()
{
  std::cout << "Setting camera zoom: " << cameraZoomPercent << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::CameraZoom, cameraZoomPercent);
}

void Controller::sendCameraFocus()
{
  std::cout << "Setting camera focus: " << cameraFocusPercent << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::CameraFocus, cameraFocusPercent);
}

void Controller::updateSpeedGracefully()
{
  int change = std::abs(motorSpeed - motorSpeedTarget);

  // Limit the change of speed
  if (change > MOTOR_SPEED_GRACE_LIMIT) {
    change = MOTOR_SPEED_GRACE_LIMIT;

    if (!motorSpeedUpdateTimer) {
      motorSpeedUpdateTimer = std::make_shared<Timer>(eventLoop);
    }

    motorSpeedUpdateTimer->start(1000/THROTTLE_FREQ_SPEED_TURN, [this]() {
      updateSpeedGracefully();
    });
  } else {
    // If no need to limit (we hit the target), make sure the timer is off
    if (motorSpeedUpdateTimer) {
      motorSpeedUpdateTimer->stop();
    }
  }

  if (motorSpeedTarget > motorSpeed) {
    motorSpeed += change;
  } else {
    motorSpeed -= change;
  }

  sendSpeedTurn(motorSpeed, motorTurn);
}

void Controller::updateCameraPeriodically()
{
  // Experimented value
  const double scale = 0.03;
  bool sendUpdate = false;

  // X
  double movementX = padCameraXPosition * scale * -1;
  double oldValueX = cameraX;

  cameraX += movementX;
  cameraX = std::max(-90.0, std::min(cameraX, 90.0));

  if (std::abs(oldValueX - cameraX) > 0.5) {
    // Update will be handled by derived class
    sendUpdate = true;
  }

  // Y
  double movementY = padCameraYPosition * scale;
  double oldValueY = cameraY;

  cameraY -= movementY;
  cameraY = std::max(-90.0, std::min(cameraY, 90.0));

  if (std::abs(oldValueY - cameraY) > 0.5) {
    // Update will be handled by derived class
    sendUpdate = true;
  }

  if (sendUpdate) {
    sendCameraXY();
  }
}

void Controller::axisChanged(int axis, std::uint16_t value)
{
  const int oversteer = 75;

  switch(axis) {
  case JOYSTICK_AXIS_STEER:
    // Scale to +-100% with +-oversteering
    motorTurn = static_cast<int>((2 * 100 + 2 * oversteer) * (value/256.0) - (100 + oversteer));

    motorTurn = std::max(-100, std::min(motorTurn, 100));

    // Send updated values
    sendSpeedTurn(motorSpeed, motorTurn);
    break;

  case JOYSTICK_AXIS_THROTTLE:
    // Scale to +-100
    motorSpeedTarget = static_cast<int>(100 * (value/256.0));
    if (motorReverse) {
      motorSpeedTarget *= -1;
    }

    updateSpeedGracefully();
    break;

  case 2:
    padCameraXPosition = value - 128;
    break;

  case 3:
    padCameraYPosition = value - 128;
    break;

  default:
    std::cout << "Joystick: Unhandled axis: " << axis << std::endl;
    break;
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
