/*
 * Copyright 2025-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Event.h"
#include <functional>
#include <chrono>
#include <asio.hpp>
#include <memory>

// Simple timer class to replace QTimer
class Timer : public std::enable_shared_from_this<Timer> {
public:
  // Takes EventLoop reference for access to io_context
  Timer(EventLoop& eventLoop)
    : timer(eventLoop.context()), running(false), repeat(false), interval(0) {}

  // Add a destructor that cancels any pending operations
  ~Timer() {
    stop();
  }

  // Start the timer with given milliseconds
  // If repeat is true, timer will restart automatically
  void start(int milliseconds, std::function<void()> callback, bool repeat = false) {
    stop();  // Cancel any pending operations

    running = true;
    this->repeat = repeat;
    this->callback = callback;
    this->interval = milliseconds;

    // Set up a one-shot timer
    timer.expires_after(std::chrono::milliseconds(milliseconds));
    timer.async_wait(std::bind(&Timer::handle_timeout, this,
                               std::placeholders::_1));
  }

  // Stop the timer
  void stop() {
    running = false;
    timer.cancel();
  }

  // Check if timer is active
  bool isActive() const {
    return running;
  }

private:
  // Private method to handle timeouts
  void handle_timeout(const asio::error_code& error) {
    if (!error && running) {
      if (callback) callback();

      if (repeat && running) {
        // Start a new timer instead of recursive call
        timer.expires_after(std::chrono::milliseconds(interval));
        timer.async_wait(std::bind(&Timer::handle_timeout, this,
                                   std::placeholders::_1));
      } else {
        running = false;
      }
    }
  }

  asio::steady_timer timer;
  std::function<void()> callback;
  bool running;
  bool repeat;
  int interval;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/