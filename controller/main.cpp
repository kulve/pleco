/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include <iostream>
#include <cstdlib>
#include <filesystem>

#include "Controller.h"
#include "UI-sdl.h"

extern "C" const char* __lsan_default_options() {
  // You can combine multiple options with colons
  return "suppressions=/home/kulve/projects/pleco/controller/lsan_suppressions.txt:print_suppressions=1";
}

int main(int argc, char *argv[])
{
  // Create the event loop
  EventLoop eventLoop;

  // Create the controller
  Controller controller(eventLoop, argc, argv);
  UI_Sdl ui(controller, argc, argv);

  // Parse command line arguments
  if (argc > 1) {
    std::string arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h") {
      std::filesystem::path exePath(argv[0]);
      std::cout << "Usage: " << exePath.filename().string() << " [ip of relay server]" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Create the GUI
  ui.createGUI();

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

  // Run the event loop in a separate thread
  controller.start();

  ui.run();

  controller.stop();

  return EXIT_SUCCESS;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/