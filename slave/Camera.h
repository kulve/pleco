/*
 * Copyright 2015-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>

class Camera
{
 public:
  Camera(void);
  ~Camera(void);

  bool init(void);
  bool setBrightness(std::uint8_t value);
  bool setZoom(std::uint8_t value);
  bool setFocus(std::uint8_t value);

 private:
  int fd;
  bool auto_focus;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
