/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Slave.h"
#include "Event.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>
#include <memory>
#include <asio.hpp>

int main(int argc, char *argv[])
{
  // Create the event loop for async operations
  EventLoop eventLoop;

  // Create the slave object with the event loop
  std::unique_ptr<Slave> slave = std::make_unique<Slave>(eventLoop, argc, argv);

  // Parse command line arguments
  bool showHelp = false;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      showHelp = true;
      break;
    }
  }

  if (showHelp) {
    std::filesystem::path execPath(argv[0]);
    std::cout << "Usage: " << execPath.filename().string() << " [ip of relay server]" << std::endl;
    return 0;
  }

  // Initialize the slave
  if (!slave->init()) {
    std::cerr << "Failed to init slave class. Exiting..." << std::endl;
    return 1;
  }

  // Determine relay server address
  std::string relay = "127.0.0.1";
  char* envRelay = std::getenv("PLECO_RELAY_IP");

  if (argc > 1) {
    relay = argv[1];
  } else if (envRelay != nullptr) {
    relay = envRelay;
  }

  // Resolve the hostname
  try {
    // We can use our event loop's io_context for resolution
    asio::ip::tcp::resolver resolver(eventLoop.context());
    auto endpoints = resolver.resolve(relay, "");

    if (endpoints.empty()) {
      std::cerr << "Failed to get IP for " << relay << std::endl;
      return 1;
    }

    // Use the first resolved address
    asio::ip::address addr = endpoints.begin()->endpoint().address();
    slave->connect(addr.to_string(), 8500);

    std::cout << "Connected to relay at " << addr.to_string() << ":8500" << std::endl;

    // Run the event loop - this will block until the application is done
    eventLoop.run();

    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << "Error resolving hostname: " << e.what() << std::endl;
    return 1;
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
