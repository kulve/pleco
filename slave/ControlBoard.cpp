/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "ControlBoard.h"
#include "Timer.h"
#include "Event.h"

#include <iostream>
#include <cstring>
#include <string>
#include <memory>
#include <functional>

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>          // errno
#include <string.h>         // strerror

// How many characters to read from the Control Board
constexpr size_t CB_BUFFER_SIZE = 1;

ControlBoard::ControlBoard(EventLoop& eventLoop, const std::string& serialDevice):
  serial_port(eventLoop.context()),
  serialDevice(serialDevice),
  serialData(),
  enabled(false),
  reopenTimer(nullptr),
  wdgTimer(nullptr)
{
  // Create timers
  reopenTimer = std::make_shared<Timer>(eventLoop);
  wdgTimer = std::make_shared<Timer>(eventLoop);
}

ControlBoard::~ControlBoard()
{
  closeSerialDevice();

  // Timers will be automatically cleaned up when their shared_ptrs are destroyed
}

void ControlBoard::closeSerialDevice(void)
{
  std::cout << "in " << __FUNCTION__ << std::endl;

  if (serial_port.is_open()) {
    std::cout << "in " << __FUNCTION__ << ": aborting" << std::endl;

    // Cancel any ongoing asynchronous operations
    asio::error_code ec;
    // NOLINTNEXTLINE(bugprone-unused-return-value)
    serial_port.cancel(ec);
    if (ec) {
      std::cerr << "Error canceling operations: " << ec.message() << std::endl;
    }

    std::cout << "in " << __FUNCTION__ << ": closing" << std::endl;
    // NOLINTNEXTLINE(bugprone-unused-return-value)
    serial_port.close(ec);
    if (ec) {
      std::cerr << "Error closing operations: " << ec.message() << std::endl;
    }  }

  std::cout << "out " << __FUNCTION__ << std::endl;
}

bool ControlBoard::init(void)
{
  // Enable Control Board connection
  if (!openSerialDevice()) {
    std::cerr << "Failed to open and setup serial port" << std::endl;
    return false;
  }

  enabled = true;
  return true;
}

void ControlBoard::readPendingSerialData(void)
{
  std::vector<std::uint8_t> buffer(128);

  asio::error_code ec;
  std::size_t bytes_read = serial_port.read_some(asio::buffer(buffer), ec);

  if (ec) {
    portError(ec.value());
    return;
  }

  if (bytes_read > 0) {
    // Append new data to our serialData buffer
    serialData.insert(serialData.end(), buffer.begin(), buffer.begin() + bytes_read);

    // If no new data coming from the serial port in 2 seconds, reopen
    // the tty device
    wdgTimer->start(2000, [this]() { reopenSerialDevice(); });

    parseSerialData();
  }

  // Continue reading asynchronously
  serial_port.async_read_some(
    asio::buffer(buffer),
    [this, buffer = std::move(buffer)](const asio::error_code& error, std::size_t bytes_transferred) {
      if (error) {
        portError(error.value());
        return;
      }

      if (bytes_transferred > 0) {
        // Append new data to our serialData buffer
        serialData.insert(serialData.end(), buffer.begin(), buffer.begin() + bytes_transferred);

        // Reset watchdog timer
        wdgTimer->start(2000, [this]() { reopenSerialDevice(); });

        parseSerialData();
      }

      // Continue reading asynchronously
      readPendingSerialData();
    }
  );
}

void ControlBoard::portError(int error)
{
  std::cerr << __FUNCTION__ << ": Socket error: " << error << std::endl;
}

void ControlBoard::portDisconnected(void)
{
  std::cerr << __FUNCTION__ << ": Socket disconnected" << std::endl;
}

void ControlBoard::reopenSerialDevice(void)
{
  std::cout << "in " << __FUNCTION__ << std::endl;
  std::cout << "Closing" << std::endl;
  closeSerialDevice();
  std::cout << "Opening" << std::endl;
  openSerialDevice();
  std::cout << "wdg" << std::endl;

  // If no new data coming from the serial port in 2 seconds, reopen
  // the tty device
  wdgTimer->start(2000, [this]() { reopenSerialDevice(); });
  std::cout << "out " << __FUNCTION__ << std::endl;
}

bool ControlBoard::openSerialDevice(void)
{
  // Open device
  int fd = open(serialDevice.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open Control Board device (" << serialDevice << "): "
              << strerror(errno) << std::endl;

    // Launch a timer and try to open again
    reopenTimer->start(1000, [this]() { reopenSerialDevice(); });

    return false;
  }

  struct termios newtio;

  // clear struct for new port settings
  bzero(&newtio, sizeof(newtio));

  // control mode flags
  newtio.c_cflag = CS8 | CLOCAL | CREAD;

  // input mode flags
  newtio.c_iflag = 0;

  // output mode flags
  newtio.c_oflag = 0;

  // local mode flags
  newtio.c_lflag = 0;

  // set input/output speeds
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);

  // now clean the serial line and activate the settings
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &newtio);

  // Assign the file descriptor to our ASIO stream_descriptor
  asio::error_code ec;
  // NOLINTNEXTLINE(bugprone-unused-return-value)
  serial_port.assign(fd, ec);
  if (ec) {
    std::cerr << "Failed to assign file descriptor to stream_descriptor: "
              << ec.message() << std::endl;
    close(fd);
    return false;
  }

  // Start asynchronous read operation
  readPendingSerialData();

  return true;
}

void ControlBoard::parseSerialData(void)
{
  if (serialData.size() < CB_BUFFER_SIZE) {
    // Not enough data
    return;
  }

  // Check for a full message ending with \n
  auto newline_pos = std::find(serialData.begin(), serialData.end(), '\n');
  if (newline_pos == serialData.end()) {
    // No complete message yet
    return;
  }

  // Extract the message (excluding newline)
  std::string message(serialData.begin(), newline_pos);

  // Remove CR if present
  if (!message.empty() && message.back() == '\r') {
    message.pop_back();
  }

  std::cout << __FUNCTION__ << " have msg: " << message << std::endl;

  // Remove processed data from buffer
  serialData.erase(serialData.begin(), newline_pos + 1);

  // Parse temperature
  if (message.compare(0, 5, "tmp: ") == 0) {
    std::string value_str = message.substr(5);
    std::uint16_t value = std::stoi(value_str);

    std::cout << __FUNCTION__ << " Temperature: " << value << std::endl;

    if (temperatureCallback) {
      temperatureCallback(value);
    }
  } else if (message.compare(0, 5, "dst: ") == 0) {
    std::string value_str = message.substr(5);
    std::uint16_t value = std::stoi(value_str);

    std::cout << __FUNCTION__ << " Distance: " << value << std::endl;

    if (distanceCallback) {
      distanceCallback(value);
    }
  } else if (message.compare(0, 5, "amp: ") == 0) {
    std::string value_str = message.substr(5);
    std::uint16_t value = std::stoi(value_str);

    std::cout << __FUNCTION__ << " Current consumption: " << value << std::endl;

    if (currentCallback) {
      currentCallback(value);
    }
  } else if (message.compare(0, 5, "vlt: ") == 0) {
    std::string value_str = message.substr(5);
    std::uint16_t value = std::stoi(value_str);

    std::cout << __FUNCTION__ << " Battery voltage: " << value << std::endl;

    if (voltageCallback) {
      voltageCallback(value);
    }
  } else if (message.compare(0, 3, "d: ") == 0) {
    std::string debug_msg = message.substr(3);

    if (debugCallback) {
      debugCallback(debug_msg);
    }
  }

  // Check if there are more complete messages in the buffer
  if (!serialData.empty()) {
    parseSerialData();
  }
}

void ControlBoard::setPWMFreq(std::uint32_t freq)
{
  if (!enabled) {
    std::cerr << __FUNCTION__ << ": Not enabled" << std::endl;
    return;
  }

  std::string cmd = "pwm_frequency " + std::to_string(freq);
  writeSerialData(cmd);
}

void ControlBoard::stopPWM(std::uint8_t pwm)
{
  if (!enabled) {
    std::cerr << __FUNCTION__ << ": Not enabled" << std::endl;
    return;
  }

  std::string cmd = "pwm_stop " + std::to_string(pwm);
  writeSerialData(cmd);
}

void ControlBoard::setPWMDuty(std::uint8_t pwm, std::uint16_t duty)
{
  if (!enabled) {
    std::cerr << __FUNCTION__ << ": Not enabled" << std::endl;
    return;
  }

  if (duty > 10000) {
    std::cerr << __FUNCTION__ << ": Duty out of range: " << duty << std::endl;
    return;
  }

  std::string cmd = "pwm_duty " + std::to_string(pwm) + " " + std::to_string(duty);
  writeSerialData(cmd);
}

void ControlBoard::setGPIO(std::uint16_t gpio, std::uint16_t enable)
{
  std::string cmd;

  // HACK: Pretending that GPIO 0 means the led
  if (gpio == 0) {
    cmd += "led ";
  }
  else {
    cmd += "gpio " + std::string(1, '0' + gpio) + " ";
  }

  cmd += enable ? "1" : "0";

  writeSerialData(cmd);
}

void ControlBoard::sendPing(void)
{
  std::string cmd = "ping";
  writeSerialData(cmd);
}

void ControlBoard::writeSerialData(const std::string& cmd)
{
  if (!serial_port.is_open()) {
    // Try not to write if the serial port is not (yet) open
    return;
  }

  std::string data = cmd + "\r";

  asio::error_code ec;
  asio::write(serial_port, asio::buffer(data), ec);

  if (ec) {
    std::cerr << "Failed to write command to ControlBoard: " << ec.message() << std::endl;
    closeSerialDevice();
    openSerialDevice();
  }
}

void ControlBoard::setDebugCallback(DebugCallback callback)
{
  debugCallback = callback;
}

void ControlBoard::setTemperatureCallback(ValueCallback callback)
{
  temperatureCallback = callback;
}

void ControlBoard::setDistanceCallback(ValueCallback callback)
{
  distanceCallback = callback;
}

void ControlBoard::setCurrentCallback(ValueCallback callback)
{
  currentCallback = callback;
}

void ControlBoard::setVoltageCallback(ValueCallback callback)
{
  voltageCallback = callback;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/