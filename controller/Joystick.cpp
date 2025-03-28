/*
 * Copyright 2013-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Joystick.h"
#include "Timer.h"

#include <sys/stat.h>        // open
#include <fcntl.h>           // open
#include <unistd.h>          // close
#include <string.h>          // strerror
#include <errno.h>           // errno
#include <iostream>

#define MAX_FEATURE_COUNT   32

struct joystick_st {
  std::string name;
  std::int16_t remap[2][MAX_FEATURE_COUNT];
};

static joystick_st supported[] = {
  {"generic default joystick without axis mapping", {{-1}}},
  {"COBRA M5", {{-1}}}
};

/*
 * Constructor for the Joystick
 */
Joystick::Joystick(EventLoop& eventLoop)
  : eventLoop(eventLoop),
    joystickDesc(eventLoop.context()),
    enabled(false),
    fd(-1),
    joystick(0),
    axisCallback(nullptr),
    buttonCallback(nullptr)
{
  // Initialize arrays
  axis_names.fill(nullptr);
  button_names.fill(nullptr);

  // Initialize mapping tables to -1 (unmapped)
  for (std::uint8_t j = 1; j < sizeof(supported)/sizeof(supported[0]); ++j) {
    for (int t = 0; t < 2; ++t) {
      for (int f = 0; f < MAX_FEATURE_COUNT; ++f) {
        supported[j].remap[t][f] = -1;
      }
    }
  }

  // Remap for Cobra M5
  supported[1].remap[0][8] = JOYSTICK_BTN_REVERSE;
  supported[1].remap[1][3] = JOYSTICK_AXIS_STEER;
  supported[1].remap[1][2] = JOYSTICK_AXIS_THROTTLE;
}

/*
 * Destructor for the Joystick
 */
Joystick::~Joystick()
{
  if (enabled) {
    // Cancel any pending async operations
    asio::error_code ec;
    joystickDesc.cancel(ec);
    if (ec) {
      std::cerr << "Error canceling joystick operations: " << ec.message() << std::endl;
    }

    // Close the file descriptor
    close(fd);
    enabled = false;
  }
}

/*
 * Set callbacks
 */
void Joystick::setAxisCallback(AxisCallback callback)
{
  axisCallback = callback;
}

void Joystick::setButtonCallback(ButtonCallback callback)
{
  buttonCallback = callback;
}

/*
 * Init Joystick. Returns false on failure
 */
bool Joystick::init(const std::string& inputDevicePath)
{
  // Open the input device using traditional open()
  fd = open(inputDevicePath.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open joystick device: " << inputDevicePath << " - " << strerror(errno) << std::endl;
    return false;
  }

  if (ioctl(fd, JSIOCGNAME(JOYSTICK_NAME_LEN), name) == -1) {
    std::cerr << "Failed to get joystick name: " << strerror(errno) << std::endl;
    close(fd);
    return false;
  }
  std::cout << "Detected joystick: " << name << std::endl;

  std::string jstr(name);
  for (std::uint8_t j = 1; j < sizeof(supported)/sizeof(supported[0]); ++j) {
    if (jstr.find(supported[j].name) != std::string::npos) {
      joystick = j;
      std::cout << "Found joystick mappings for " << supported[j].name << std::endl;
      break;
    }
  }

  // Assign the file descriptor to the ASIO stream descriptor
  asio::error_code ec;
  joystickDesc.assign(fd, ec);
  if (ec) {
    std::cerr << "Failed to assign joystick descriptor: " << ec.message() << std::endl;
    close(fd);
    return false;
  }

  // Start async read operation
  enabled = true;
  readPendingInputData();

  return true;
}

/*
 * Read data from the input device
 */
void Joystick::readPendingInputData()
{
  if (!enabled) return;

  // Start an asynchronous read for a joystick event
  joystickDesc.async_read_some(
    asio::buffer(&event, sizeof(js_event)),
    [this](const asio::error_code& ec, std::size_t bytes_transferred) {
      if (ec) {
        if (ec != asio::error::operation_aborted) {
          std::cerr << "Joystick read error: " << ec.message() << std::endl;
          enabled = false;
        }
        return;
      }

      if (bytes_transferred < sizeof(js_event)) {
        std::cerr << "Too few bytes read: " << bytes_transferred << std::endl;
        readPendingInputData();  // Continue reading
        return;
      }

      // Process the joystick event
      if (event.type != 1 && event.type != 2) {
        // Ignore initialization events (bit 7 is set)
        readPendingInputData();
        return;
      }

      int ab_number = event.number;
      if (ab_number >= MAX_FEATURE_COUNT) {
        std::cerr << "Axis/button number too high, ignoring: " << ab_number << std::endl;
        readPendingInputData();
        return;
      }

      if (joystick > 0) {
        int tmp = supported[joystick].remap[event.type - 1][ab_number];
        if (tmp == -1) {
          // Unmapped axis/button, continue reading
          readPendingInputData();
          return;
        }

        ab_number = tmp;
      }

      // Handle button press
      if (event.type == 1) {
        if (buttonCallback) {
          buttonCallback(ab_number, static_cast<std::uint16_t>(event.value));
        }
      }
      // Handle axis movement
      else if (event.type == 2) {
        std::uint16_t value = static_cast<std::uint16_t>(event.value / 256.0 + 128);
        if (axisCallback) {
          axisCallback(ab_number, value);
        }
      }

      // Continue reading
      readPendingInputData();
    });
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
