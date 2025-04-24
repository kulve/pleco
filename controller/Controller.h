/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include "Event.h"
#include "Timer.h"
#include "Transmitter.h"
#include "IVideoReceiver.h"
#include "AudioReceiver.h"
#include "Joystick.h"
#include "Stats.h"

// Defines for the stats to be used with raw data pointer
// These must match the order in the Stats::Container class
#define CTRL_STATS_RTT                0
#define CTRL_STATS_RESEND_TIMEOUT     1
#define CTRL_STATS_RESENT_PACKETS     2
#define CTRL_STATS_UPTIME             3
#define CTRL_STATS_LOAD_AVG           4
#define CTRL_STATS_WLAN_STRENGTH      5
#define CTRL_STATS_DISTANCE           6
#define CTRL_STATS_TEMPERATURE        7
#define CTRL_STATS_CURRENT            8
#define CTRL_STATS_VOLTAGE            9
#define CTRL_STATS_TURN              10
#define CTRL_STATS_SPEED             11
#define CTRL_STATS_PAYLOAD_RX        12
#define CTRL_STATS_PAYLOAD_TX        13
#define CTRL_STATS_TOTAL_RX          14
#define CTRL_STATS_TOTAL_TX          15
#define CTRL_STATS_CONNECTION_STATUS 16
#define CTRL_STATS_COUNT             17

class Controller
{
 public:
  Controller(EventLoop& loop, IVideoReceiver *vr, AudioReceiver *ar);
  virtual ~Controller();

  // Start the controller's event loop in a separate thread
  void start();
  // Stop the controller's event loop
  void stop();
  // Connect to the relay server
  void connect(const std::string& host, std::uint16_t port);

  // Set functions for the UI
  void setCameraZoom();
  void setCameraFocus();
  void setVideoQuality();
  void setCameraX(int degree);
  void setCameraY(int degree);
  void setCameraZoom(int CameraZoom);
  void setCameraFocus(int CameraFocus);
  void setVideoQuality(int quality);
  void setSpeedTurn(int speed, int turn);
  void setVideoSource(int source);
  void setLed(bool enable);
  void setVideo(bool enable);
  void setAudio(bool enable);
  void setHalfSpeed(bool enable);

  // Get functions for the UI
  void getStats(int32_t *out) const;

  // Get/release latest video frame
  bool videoFrameGet(IVideoReceiver::FrameData& frameData);
  void videoFrameRelease(IVideoReceiver::FrameData& frameData);

 private:
  void sendCameraXY();
  void sendCameraXYIfPending();
  void sendSpeedTurnIfPending();
  void updateSpeedGracefully();
  void axisChanged(int axis, std::uint16_t value);
  void updateCameraPeriodically();
  void buttonChanged(int axis, std::uint16_t value);
  void updateValue(std::uint8_t type, std::uint16_t value);
  void updatePeriodicValue(std::uint8_t type, std::uint16_t value);

  std::thread eventLoopThread;
  bool eventLoopRunning = false;

  // Components
  std::unique_ptr<Transmitter> transmitter;

  std::unique_ptr<IVideoReceiver> vr;
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

  Stats::Container stats;

  // Motor control
  int calibrateSpeed;
  int calibrateTurn;
  int speed;
  int turn;

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