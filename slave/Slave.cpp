/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Slave.h"
#include "Transmitter.h"
#include "VideoSender.h"
#include "AudioSender.h"
#include "Message.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <cstring>
#include <memory>

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

Slave::Slave(EventLoop& eventLoop, int, char **):
  eventLoop(eventLoop),
  oldSpeed(0), oldTurn(0), oldDirectionLeft(0), oldDirectionRight(0),
  running(true)
{
  // No initialization in constructor - all setup happens in init()
}

Slave::~Slave()
{
  // Smart pointers handle cleanup automatically
  running = false;
}

bool Slave::init(void)
{
  // Check on which hardware we are running based on the info in /proc/cpuinfo.
  // Defaulting to Generic X86
  std::string hardwareName("");
  std::vector<std::string> detectFiles = {
    "/proc/cpuinfo",
    "/proc/device-tree/model"
  };

  for (const auto& filename : detectFiles) {
    std::ifstream file(filename);
    if (file.is_open()) {
      std::cout << "Reading " << filename << std::endl;
      std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

      if (content.find("Gumstix Overo") != std::string::npos) {
        std::cout << "Detected Gumstix Overo" << std::endl;
        hardwareName = "gumstix_overo";
        break;
      } else if (content.find("BCM2708") != std::string::npos) {
        std::cout << "Detected Broadcom based Raspberry Pi" << std::endl;
        hardwareName = "raspberry_pi";
        break;
      } else if (content.find("grouper") != std::string::npos) {
        std::cout << "Detected Tegra 3 based Nexus 7" << std::endl;
        hardwareName = "tegra3";
      } else if (content.find("cardhu") != std::string::npos) {
        std::cout << "Detected Tegra 3 based Cardhu or Ouya" << std::endl;
        hardwareName = "tegra3";
        break;
      } else if (content.find("jetson-tk1") != std::string::npos) {
        std::cout << "Detected Tegra K1 based Jetson TK1" << std::endl;
        hardwareName = "tegrak1";
        break;
      } else if (content.find("jetson_tx1") != std::string::npos) {
        std::cout << "Detected Tegra X1 based Jetson TX1" << std::endl;
        hardwareName = "tegrax1";
      } else if (content.find("quill") != std::string::npos) {
        std::cout << "Detected Tegra X2 based Jetson TX2" << std::endl;
        hardwareName = "tegrax2";
      } else if (content.find("Jetson Nano") != std::string::npos) {
        std::cout << "Detected Jetson Nano" << std::endl;
        hardwareName = "tegra_nano";
      } else if (content.find("GenuineIntel") != std::string::npos) {
        std::cout << "Detected X86" << std::endl;
        hardwareName = "generic_x86";
      }

      file.close();
    }
  }

  if (hardwareName.empty()) {
    std::cerr << "Failed to detect HW, guessing generic x86" << std::endl;
    hardwareName = "generic_x86";
  }

  std::cout << "Initialising hardware object: " << hardwareName << std::endl;

  hardware = std::make_unique<Hardware>(hardwareName);

  std::string tty = std::getenv("PLECO_MCU_TTY") ? std::getenv("PLECO_MCU_TTY") : "/dev/pleco-uart";
  cb = std::make_unique<ControlBoard>(eventLoop, tty);

  // FIXME: if the init fails, wait for a signal that is has succeeded (or wait it always?)
  if (!cb->init()) {
    std::cerr << "Failed to initialize ControlBoard" << std::endl;
    // CHECKME: to return false or not to return false (and do clean up)?
  } else {
    // Assuming PWM frequencies are set to correct values already at built time.
  }

  camera = std::make_unique<Camera>();
  if (camera->init()) {
    camera->setBrightness(0);
  }

  // Start a timer for sending ping to the control board
  cbPingTimer = std::make_shared<Timer>(eventLoop);
  cbPingTimer->start(100, [this]() { sendCBPing(); }, true);

  return true;
}

void Slave::connect(const std::string& host, std::uint16_t port)
{
  // Create a new transmitter
  transmitter = std::make_unique<Transmitter>(eventLoop, host, port);

  // Set up callbacks
  transmitter->setValueCallback([this](std::uint8_t type, std::uint16_t value) {
    updateValue(type, value);
  });

  transmitter->setConnectionStatusCallback([this](int status) {
    updateConnectionStatus(status);
  });

  transmitter->initSocket();

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);

  // Start timer for sending system statistics (wlan signal, cpu load) periodically
  statsTimer = std::make_shared<Timer>(eventLoop);
  statsTimer->start(1000, [this]() { sendSystemStats(); }, true);

  // Create and enable sending video
  vs = std::make_unique<VideoSender>(eventLoop, hardware.get(), 0);

  // Check if video1 exists and camera is not overridden with env variable, and then create a 2nd stream
  std::filesystem::path v1path("/dev/video1");
  if (std::filesystem::exists(v1path) && !std::getenv("PLECO_SLAVE_CAMERA")) {
    vs2 = std::make_unique<VideoSender>(eventLoop, hardware.get(), 1);
  }

  // Create and enable sending audio
  as = std::make_unique<AudioSender>(hardware.get());

  // Set up callbacks for video and audio data
  vs->setVideoCallback([this](std::vector<std::uint8_t>* video, std::uint8_t index) {
    transmitter->sendVideo(video, index);
  });

  if (vs2) {
    vs2->setVideoCallback([this](std::vector<std::uint8_t>* video, std::uint8_t index) {
      transmitter->sendVideo(video, index);
    });
  }

  as->setAudioCallback([this](std::vector<std::uint8_t>* audio) {
    transmitter->sendAudio(audio);
  });

  // Set up control board callbacks
  cb->setDebugCallback([this](const std::string& debug) {
    auto str = new std::string(debug);
    transmitter->sendDebug(str);
  });

  cb->setDistanceCallback([this](std::uint16_t value) {
    cbDistance(value);
  });

  cb->setTemperatureCallback([this](std::uint16_t value) {
    cbTemperature(value);
  });

  cb->setCurrentCallback([this](std::uint16_t value) {
    cbCurrent(value);
  });

  cb->setVoltageCallback([this](std::uint16_t value) {
    cbVoltage(value);
  });
}

void Slave::run()
{
  // No need to do anything - the event loop runs from main.cpp
  running = true;
}

void Slave::sendSystemStats(void)
{
  // Check signal strength from wireless
  {
    std::ifstream file("/sys/class/net/wlan0/wireless/link");
    if (file.is_open()) {
      std::string line;
      std::getline(file, line);
      unsigned int link = std::stoul(line);
      // Link is 0-70, convert to 0-100%
      std::uint16_t signal = static_cast<std::uint16_t>(link/70.0 * 100);

      transmitter->sendPeriodicValue(MessageSubtype::SignalStrength, signal);
      file.close();
    } else {
      std::ifstream wireless("/proc/net/wireless");
      if (wireless.is_open()) {
        std::string line;
        std::getline(wireless, line); // skip header line
        std::getline(wireless, line); // skip header line
        std::getline(wireless, line); // get content line

        std::uint16_t signal = 0;
        if (!line.empty()) {
          std::istringstream iss(line);
          std::string token;

          // Skip interface name
          iss >> token;

          // Read signal quality
          iss >> token; // skip status
          iss >> token;

          // Remove trailing period
          if (token.back() == '.') {
            token.pop_back();
          }

          signal = static_cast<std::uint16_t>(std::stoul(token));
        }
        transmitter->sendPeriodicValue(MessageSubtype::SignalStrength, signal);
        wireless.close();
      }
    }
  }

  // Check CPU load from /proc/loadavg
  {
    std::ifstream file("/proc/loadavg");
    if (file.is_open()) {
      std::string line;
      std::getline(file, line);
      double loadavg = std::stod(line.substr(0, line.find(' ')));
      // Load avg is double, send as 100x integer
      std::uint16_t load = static_cast<std::uint16_t>(loadavg * 100);

      transmitter->sendPeriodicValue(MessageSubtype::CPUUsage, load);
      file.close();
    }
  }

  // Check uptime from /proc/uptime
  {
    std::ifstream file("/proc/uptime");
    if (file.is_open()) {
      std::string line;
      std::getline(file, line);
      double uptime = std::stod(line.substr(0, line.find(' ')));
      // Uptime is seconds as double, send seconds as uint
      std::uint16_t uptimeVal = static_cast<std::uint16_t>(uptime);

      transmitter->sendPeriodicValue(MessageSubtype::Uptime, uptimeVal);
      file.close();
    }
  }

  // Check system temperature
  {
    std::ifstream file("/sys/devices/virtual/hwmon/hwmon0/temp1_input");
    if (file.is_open()) {
      std::string line;
      std::getline(file, line);
      unsigned int mtemp = std::stoul(line);
      // Temperature is in millicelsius, convert to hundreds of celsius
      std::uint16_t temp = static_cast<std::uint16_t>(mtemp / 10.0);

      transmitter->sendPeriodicValue(MessageSubtype::Temperature, temp);
      file.close();
    }
  }
}

void Slave::updateValue(std::uint8_t type, std::uint16_t value)
{
  std::cout << "in updateValue, type: " << Message::getSubTypeStr(type)
            << ", value: " << value << std::endl;

  switch (type) {
  case MessageSubtype::EnableLED:
    cb->setGPIO(CB_GPIO::LED1, value);
    cb->setGPIO(CB_GPIO::HEAD_LIGHTS, value);
    break;
  case MessageSubtype::EnableVideo:
    parseSendVideo(value);
    break;
  case MessageSubtype::EnableAudio:
    parseSendAudio(value);
    break;
  case MessageSubtype::VideoSource:
    vs->setVideoSource(value);
    if (vs2) {
      vs2->setVideoSource(value);
    }
    break;
  case MessageSubtype::CameraXY:
    parseCameraXY(value);
    break;
  case MessageSubtype::CameraZoom:
    camera->setZoom(value);
    break;
  case MessageSubtype::CameraFocus:
    camera->setFocus(value);
    break;
  case MessageSubtype::SpeedTurn:
    parseSpeedTurn(value);
    break;
  case MessageSubtype::VideoQuality:
    parseVideoQuality(value);
    break;
  default:
    std::cerr << "updateValue: Unknown type: " << Message::getSubTypeStr(type) << std::endl;
  }
}

void Slave::updateConnectionStatus(int status)
{
  std::cout << "in updateConnectionStatus, status: " << status << std::endl;

  if (status == CONNECTION_STATUS_LOST) {
    std::cout << "in updateConnectionStatus, Stop all PWM" << std::endl;

    // Stop all motors
    for (std::uint8_t i = 1; i <= CB_PWM::PWM8; ++i) {
      cb->stopPWM(i);
    }
    oldSpeed = 0;
    oldTurn = 0;

    // Stop motor drivers
    // FIXME: only in NOR, not in pleco
    cb->setGPIO(CB_GPIO::SPEED_ENABLE_LEFT, 0);
    cb->setGPIO(CB_GPIO::SPEED_ENABLE_RIGHT, 0);

    // Stop sending video
    parseSendVideo(0);

    // Stop sending audio
    parseSendAudio(0);
  }

  // FIXME: if connection restored (or just ok for the first time), send status to controller?
}

void Slave::cbTemperature(std::uint16_t value)
{
  transmitter->sendPeriodicValue(MessageSubtype::Temperature, value);
}

void Slave::cbDistance(std::uint16_t value)
{
  transmitter->sendPeriodicValue(MessageSubtype::Distance, value);
}

void Slave::cbCurrent(std::uint16_t value)
{
  transmitter->sendPeriodicValue(MessageSubtype::BatteryCurrent, value);
}

void Slave::cbVoltage(std::uint16_t value)
{
  transmitter->sendPeriodicValue(MessageSubtype::BatteryVoltage, value);
}

void Slave::parseSendVideo(std::uint16_t value)
{
  vs->enableSending(value ? true : false);
  if (vs2) {
    vs2->enableSending(value ? true : false);
  }
}

void Slave::parseSendAudio(std::uint16_t value)
{
  as->enableSending(value ? true : false);
}

void Slave::parseVideoQuality(std::uint16_t value)
{
  vs->setVideoQuality(value);
  if (vs2) {
    vs2->setVideoQuality(value);
  }
}

void Slave::parseCameraXY(std::uint16_t value)
{
  std::uint16_t x, y;
  static std::uint16_t oldx = 0, oldy = 0;

  // Value is a 16 bit containing 2x 8bit values that are doubled percentages
  x = (value >> 8);
  y = (value & 0x00ff);

  // Control board expects percentages to be x100 integers
  // Servos expect 5-10% duty cycle for the 1-2ms pulses
  x = static_cast<std::uint16_t>(x * (5 / 2.0)) + 500;
  y = static_cast<std::uint16_t>(y * (5 / 2.0)) + 500;

  // Update servo positions only if value has changed
  if (x != oldx) {
    cb->setPWMDuty(CB_PWM::CAMERA_X, x);
    std::cout << "in parseCameraXY, Camera X PWM: " << x << std::endl;
    oldx = x;
  }

  if (y != oldy) {
    cb->setPWMDuty(CB_PWM::CAMERA_Y, y);
    std::cout << "in parseCameraXY, Camera Y PWM: " << y << std::endl;
    oldy = y;
  }
}

/*
 * Tank style steering. This assume one PWM for each side and full
 * duty cycle instead of the servo style 1-2ms pulses. The PWM is
 * 0-100% and a separate GPIO is used for direction (forward/reverse)
 */
void Slave::speedTurnTank(std::uint8_t speed_raw, std::uint8_t turn_raw)
{
  // Convert from 0-200 to +-100%
  std::int8_t speed = speed_raw - 100;
  std::int8_t turn = turn_raw - 100;

  float speed_left = speed;
  float speed_right = speed;
  float turn_adjustment = speed * ((abs(turn)*2)/100.0);

  // Turning right, decrease the speed of the right wheel
  if (turn > 0) {
    speed_right -= turn_adjustment;
  } else if (turn < 0) {
    // Turning left, decrease the speed of the left wheel
    speed_left -= turn_adjustment;
  }

  std::uint8_t direction_left = 1;
  std::uint8_t direction_right = 1;

  if (speed_left < 0) {
    speed_left *= -1;
    direction_left = 0;
  }

  if (speed_right < 0) {
    speed_right *= -1;
    direction_right = 0;
  }

  // Update servo/ESC positions only if the value has changed
  if (speed != oldSpeed || turn != oldTurn) {

    // Enable/disable motor drivers
    if ((speed != 0 && oldSpeed == 0) || (speed == 0 && oldSpeed != 0)) {
      std::uint16_t enable = speed ? 1 : 0;
      cb->setGPIO(CB_GPIO::SPEED_ENABLE_LEFT, enable);
      cb->setGPIO(CB_GPIO::SPEED_ENABLE_RIGHT, enable);

      // Make sure to set direction always after enabling motors.
      if (enable) {
        oldDirectionLeft = -1;
        oldDirectionRight = -1;
      }
    }

    // Apply direction (changes) only if the speed is low for safety reasons
    if (direction_left != oldDirectionLeft && speed_left < 30) {
      cb->setGPIO(CB_GPIO::DIRECTION_LEFT, direction_left);
    }
    if (direction_right != oldDirectionRight && speed_right < 30) {
      cb->setGPIO(CB_GPIO::DIRECTION_RIGHT, direction_right);
    }

    cb->setPWMDuty(CB_PWM::SPEED_LEFT, static_cast<std::uint16_t>(speed_left * 100));
    cb->setPWMDuty(CB_PWM::SPEED_RIGHT, static_cast<std::uint16_t>(speed_right * 100));

    std::cout << "in speedTurnTank, Speed PWM left: " << speed_left
              << ", right: " << speed_right << std::endl;

    oldSpeed = speed;
    oldTurn = turn;
    oldDirectionLeft = direction_left;
    oldDirectionRight = direction_right;
  }
}

/*
 * Ackerman steering. This assumes Rock Crawler 4 wheel drive with all
 * wheel turning and driven by the standard servo PWM logic (1-2 ms
 * pulses).
 */
void Slave::speedTurnAckerman(std::uint8_t speed_raw, std::uint8_t turn_raw)
{
  std::int16_t speed;
  std::int16_t turn;

  // Control board expects percentages to be x100 integers
  // Servos expect 5-10% duty cycle for the 1-2ms pulses
  speed = static_cast<std::int16_t>(speed_raw * (5 / 2.0)) + 500;
  turn = static_cast<std::int16_t>(turn_raw * (5 / 2.0)) + 500;

  // Update servo/ESC positions only if value has changed
  if (speed != oldSpeed) {
    cb->setPWMDuty(CB_PWM::SPEED, speed);

    std::cout << "in speedTurnAckerman, Speed PWM: " << speed << std::endl;

    // Update rear lights if slowing down
    if (speed < oldSpeed || speed < 0) {
      // Start a timer for turning off rear lights
      if (!rearLightTimer) {
        rearLightTimer = std::make_shared<Timer>(eventLoop);
      }
      rearLightTimer->start(2000, [this]() { turnOffRearLight(); });

      cb->setGPIO(CB_GPIO::REAR_LIGHTS, 1);
    }
    oldSpeed = speed;
  }

  if (turn != oldTurn) {
    cb->setPWMDuty(CB_PWM::TURN, turn);
    std::cout << "in speedTurnAckerman, Turn PWM1: " << turn << std::endl;

    if (1) { // Rock Crawler's rear wheels also turn
      // The rear wheels must be turned vice versa compared to front wheels
      // 500-1000 -> 0-500 -> 500-0 -> 1000-500
      std::uint16_t turn2 = (500 - (turn - 500)) + 500;

      cb->setPWMDuty(CB_PWM::TURN2, turn2);
      std::cout << "in speedTurnAckerman, Turn PWM2: " << turn2 << std::endl;
    }
    oldTurn = turn;
  }
}

void Slave::parseSpeedTurn(std::uint16_t value)
{
  std::int16_t speed, turn;
  bool ackerman = false;

  // Value is a 16 bit containing 2x 8bit values shifted by 100 to get positive numbers
  speed = (value >> 8);
  turn = (value & 0x00ff);

  if (ackerman) {
    speedTurnAckerman(speed, turn);
  } else {
    speedTurnTank(speed, turn);
  }
}

void Slave::turnOffRearLight(void)
{
  cb->setGPIO(CB_GPIO::REAR_LIGHTS, 0);
}

void Slave::sendCBPing(void)
{
  cb->sendPing();
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/