/*
 * Copyright 2025-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <asio.hpp>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void run();
    void stop();
    asio::io_context& context();

private:
    asio::io_context io_context;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard;
};
