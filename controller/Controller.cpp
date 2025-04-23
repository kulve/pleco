/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Controller.h"

#include <iostream>
#include <cmath>

#include "AudioReceiver.h"
#include "IVideoReceiver.h"
#include "Stats.h"
#include "Timer.h"
#include "Transmitter.h"

// Limit the frequency of sending commands affecting PWM signals
#define THROTTLE_FREQ_CAMERA_XY  50
#define THROTTLE_FREQ_SPEED_TURN 50

// Limit the motor speed change
#define MOTOR_SPEED_GRACE_LIMIT  10

Controller::Controller(EventLoop& loop, IVideoReceiver *vr, AudioReceiver *ar):
    transmitter(nullptr),
    vr(vr),
    ar(ar),
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

}

Controller::~Controller()
{
  stop();
}

void Controller::connect(const std::string& host, std::uint16_t port)
{
  if (transmitter) {
    std::cout << "Transmitter already created, doing nothing." << std::endl;
    return;
  }

  transmitter = std::make_unique<Transmitter>(eventLoop, host, port);

  std::cout << "Setting transmitter callbacks" << std::endl;

  transmitter->setRttCallback([this](int ms) {
    stats[Stats::Type::Rtt] = ms;
  });

  transmitter->setResendTimeoutCallback([this](int ms) {
    stats[Stats::Type::ResendTimeout] = ms;
  });

  transmitter->setResentPacketsCallback([this](uint32_t resendCounter) {
    stats[Stats::Type::ResentPackets] = resendCounter;
  });

  transmitter->setNetworkRateCallback([this](int payloadRx, int totalRx, int payloadTx, int totalTx) {
    stats[Stats::Type::PayloadRx] = payloadRx;
    stats[Stats::Type::PayloadTx] = payloadTx;
    stats[Stats::Type::TotalRx] = totalRx;
    stats[Stats::Type::TotalTx] = totalTx;
  });

  transmitter->setValueCallback([this](uint8_t type, uint16_t value) {
    updateValue(type, value);
  });

  transmitter->setPeriodicValueCallback([this](uint8_t type, uint16_t value) {
    updatePeriodicValue(type, value);
  });

  #if 0 /// FIXME
  transmitter->setDebugCallback([this](std::string* /*debug*/) {
    showDebug(*debug);
  });
  #endif
  transmitter->setConnectionStatusCallback([this](int status) {
    connectionStatus = status;
  });

  transmitter->setVideoCallback([this](std::vector<uint8_t>* video) {
    if (vr) vr->consumeBitStream(video);
  });

  transmitter->setAudioCallback([this](std::vector<uint8_t>* audio) {
    if (ar) ar->consumeAudio(audio);
  });

  transmitter->initSocket();

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);
}

void Controller::updateValue(std::uint8_t type, std::uint16_t value)
{
  switch(type) {
    case MessageSubtype::BatteryCurrent:
      stats[Stats::Type::Current] = value;
      break;
    case MessageSubtype::BatteryVoltage:
      stats[Stats::Type::Voltage] = value;
      break;
    case MessageSubtype::Distance:
      stats[Stats::Type::Distance] = value;
      break;
    case MessageSubtype::Temperature:
      stats[Stats::Type::Temperature] = value;
      break;
    case MessageSubtype::SignalStrength:
      stats[Stats::Type::WlanStrength] = value;
      break;
    default:
      std::cout << "Unhandled value type: " << static_cast<int>(type) << " = " << value << std::endl;
      break;
  }
}

void Controller::updatePeriodicValue(std::uint8_t type, std::uint16_t value)
{
  switch(type) {
    case MessageSubtype::CPUUsage:
      stats[Stats::Type::LoadAvg] = value;
      break;
    case MessageSubtype::Uptime:
      stats[Stats::Type::Uptime] = value;
      break;
    default:
      std::cout << "Unhandled periodic value type: " << static_cast<int>(type) << " = " << value << std::endl;
      break;
  }
}

void Controller::start() {
  if (eventLoopRunning) {
      std::cout << "Event loop already running" << std::endl;
      return;
  }

  eventLoopRunning = true;
  eventLoopThread = std::thread([this]() {
      try {
          eventLoop.run();
      } catch (const std::exception& e) {
          std::cerr << "Exception in event loop: " << e.what() << std::endl;
      }
      eventLoopRunning = false;
  });

  std::cout << "Event loop thread started" << std::endl;
}

void Controller::stop() {
  if (!eventLoopRunning) {
      return;
  }

  std::cout << "Stopping event loop..." << std::endl;
  eventLoop.stop();

  if (eventLoopThread.joinable()) {
      eventLoopThread.join();
  }
  std::cout << "Event loop stopped" << std::endl;
}

void Controller::setLed(bool enable)
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

void Controller::setVideo(bool enable)
{
  if (videoState == enable) {
    return;
  }
  videoState = enable;

  std::cout << "Video state changed: " << (videoState ? "enabled" : "disabled") << std::endl;

  if (!transmitter) return;

  if (!vr) {
    std::cerr << "Video receiver not set" << std::endl;
    videoState = false;
    return;
  }

  if (!vr->init()) {
    std::cerr << "Failed to initialize video receiver" << std::endl;
    videoState = false;
    return;
  }

  if (videoState) {
    transmitter->sendValue(MessageSubtype::EnableVideo, 1);
  } else {
    transmitter->sendValue(MessageSubtype::EnableVideo, 0);
  }
}

void Controller::setAudio(bool enable)
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

void Controller::setHalfSpeed(bool enable)
{
  if (motorHalfSpeed == enable) {
    return;
  }
  motorHalfSpeed = enable;

  std::cout << "Half speed state changed: " << (motorHalfSpeed ? "enabled" : "disabled") << std::endl;

}

void Controller::setVideoQuality(int quality)
{
  // FIXME: Do these need to be class members or simply send to slave?
  videoQuality = quality;

  std::cout << "Setting video quality: " << videoQuality << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::VideoQuality, videoQuality);
}

void Controller::setVideoSource(int source)
{

  std::cout << "Setting video source: " << source << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::VideoSource, static_cast<std::uint16_t>(source));
}

void Controller::sendCameraXYIfPending()
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
      sendCameraXYIfPending();
    });
  }

  transmitter->sendValue(MessageSubtype::CameraXY, value);
}

void Controller::sendSpeedTurnIfPending()
{
  if (speedTurnPending) {
    speedTurnPending = false;
    setSpeedTurn(speedTurnPendingSpeed, speedTurnPendingTurn);
  }
}

void Controller::setSpeedTurn(int speed, int turn)
{
  motorSpeed = speed;
  motorTurn = turn;

  if (!transmitter) return;

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
      sendSpeedTurnIfPending();
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

void Controller::setCameraZoom(int CameraZoom)
{
  cameraZoomPercent = CameraZoom;

  std::cout << "Setting camera zoom: " << cameraZoomPercent << std::endl;
  if (!transmitter) return;

  transmitter->sendValue(MessageSubtype::CameraZoom, cameraZoomPercent);
}

void Controller::setCameraFocus(int cameraFocus)
{
  cameraFocusPercent = cameraFocus;

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

  setSpeedTurn(motorSpeed, motorTurn);
}

void Controller::setCameraX(int degree)
{
  cameraX = degree;
  sendCameraXY();
}

void Controller::setCameraY(int degree)
{
  cameraY = degree;
  sendCameraXY();
}
bool Controller::videoFrameGet(IVideoReceiver::FrameData& frameData)
{
  if (vr) {
    return vr->frameGet(frameData);
  }
  return false;
}

void Controller::videoFrameRelease(IVideoReceiver::FrameData& frameData)
{
  if (vr) {
    vr->frameRelease(frameData);
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
