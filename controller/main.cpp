/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Controller.h"
#include "Controller-sdl.h"

#include <string>
#include <iostream>
#include <cstdlib>
#include <filesystem>

int main(int argc, char *argv[])
{
  // Create the controller
  Controller_sdl controller(argc, argv);

  // Parse command line arguments
  if (argc > 1) {
    std::string arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h") {
      std::filesystem::path exePath(argv[0]);
      std::cout << "Usage: " << exePath.filename().string() << " [ip of relay server]" << std::endl;
      return 0;
    }
  }

  // Create the GUI
  controller.createGUI();

  // Configure relay server address
  std::string relay = "127.0.0.1";

  // Check environment variable first
  char* env_relay = std::getenv("PLECO_RELAY_IP");
  if (env_relay != nullptr) {
    relay = env_relay;
  }

  // Command line argument overrides environment variable
  if (argc > 1) {
    relay = argv[1];
  }

  // Connect to the server
  controller.connect(relay, 12347);

  // Run the event loop
  controller.run();

  return 0;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/