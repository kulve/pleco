/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Event.h"
#include "Transmitter.h"
#include "Hardware.h"
#include "VideoSender.h"
#include "AudioSender.h"
#include "ControlBoard.h"
#include "Camera.h"

#include <string>
#include <memory>
#include <cstdint>

class Slave
{
 public:
  Slave(EventLoop& eventLoop, int argc, char** argv);
  ~Slave();

  bool init(void);
  void connect(const std::string& host, std::uint16_t port);
  void run();

 private:
  // Callbacks replacing Qt slots
  void sendSystemStats();
  void updateValue(std::uint8_t type, std::uint16_t value);
  void updateConnectionStatus(int status);
  void cbTemperature(std::uint16_t value);
  void cbDistance(std::uint16_t value);
  void cbCurrent(std::uint16_t value);
  void cbVoltage(std::uint16_t value);
  void sendCBPing();
  void turnOffRearLight();
  void speedTurnAckerman(std::uint8_t speed, std::uint8_t turn);
  void speedTurnTank(std::uint8_t speed, std::uint8_t turn);

  // Helper methods
  void parseSendVideo(std::uint16_t value);
  void parseSendAudio(std::uint16_t value);
  void parseCameraXY(std::uint16_t value);
  void parseSpeedTurn(std::uint16_t value);
  void parseVideoQuality(std::uint16_t value);

  // Event loop and timer
  EventLoop &eventLoop;
  std::shared_ptr<Timer> cbPingTimer;
  std::shared_ptr<Timer> statsTimer;
  std::shared_ptr<Timer> rearLightTimer;

  // Components
  std::unique_ptr<Transmitter> transmitter;
  std::unique_ptr<VideoSender> vs;
  std::unique_ptr<VideoSender> vs2;
  std::unique_ptr<AudioSender> as;
  std::unique_ptr<Hardware> hardware;
  std::unique_ptr<ControlBoard> cb;
  std::unique_ptr<Camera> camera;

  // State variables
  std::int16_t oldSpeed;
  std::int16_t oldTurn;
  std::uint8_t oldDirectionLeft;
  std::uint8_t oldDirectionRight;
  bool running;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
