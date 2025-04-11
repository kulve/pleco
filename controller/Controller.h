/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Event.h"
#include "Transmitter.h"
#include "VideoReceiver.h"
#include "AudioReceiver.h"
#include "Joystick.h"

#include <string>
#include <memory>
#include <cstdint>

// Forward declarations
class Timer;

class Controller
{
 public:
  Controller(EventLoop& loop, int &argc, char **argv);
  virtual ~Controller();

  virtual void createGUI() = 0;
  virtual void connect(const std::string& host, std::uint16_t port) = 0;

 protected:
  // Methods replacing Qt slots
  void updateSpeedGracefully();
  void axisChanged(int axis, std::uint16_t value);
  void updateCameraPeriodically();
  void buttonChanged(int axis, std::uint16_t value);

  // Protected helper methods
  void sendCameraXYPending();
  void sendSpeedTurnPending();
  void sendCameraZoom();
  void sendCameraFocus();
  void sendVideoQuality();
  void sendCameraXY();
  void sendSpeedTurn(int speed, int turn);
  void sendVideoSource(int source);
  void enableLed(bool enable);
  void enableVideo(bool enable);
  void enableAudio(bool enable);

  // Components
  std::unique_ptr<Joystick> joystick;
  std::unique_ptr<Transmitter> transmitter;
  std::unique_ptr<VideoReceiver> vr;
  std::unique_ptr<AudioReceiver> ar;

  // Camera state
  int padCameraXPosition;
  int padCameraYPosition;
  double cameraX;
  double cameraY;

  // Motor state
  int motorSpeedTarget;
  int motorSpeed;
  bool motorReverse;
  std::shared_ptr<Timer> motorSpeedUpdateTimer;
  int motorTurn;

  // Pending state flags
  bool cameraXYPending;
  bool speedTurnPending;
  int speedTurnPendingSpeed;
  int speedTurnPendingTurn;

  // Video and camera settings
  int videoBufferPercent;
  int videoQuality;
  int cameraFocusPercent;
  int cameraZoomPercent;

  // Feature state
  bool motorHalfSpeed;
  bool calibrate;
  bool audioState;
  bool videoState;
  bool ledState;

  // Throttling timers
  std::shared_ptr<Timer> throttleTimerCameraXY;
  std::shared_ptr<Timer> throttleTimerSpeedTurn;

  // Reference to event loop
  EventLoop& eventLoop;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/