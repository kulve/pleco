/*
 * Copyright 2025-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Event.h"

EventLoop::EventLoop()
    : work_guard(asio::make_work_guard(io_context))
{
}

EventLoop::~EventLoop()
{
    stop();
}

void EventLoop::run() {
    // Start the event processing loop
    io_context.run();
}

void EventLoop::stop() {
    // Stop the event loop
    io_context.stop();
}

asio::io_context& EventLoop::context() {
    return io_context;
}
