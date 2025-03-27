/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <cstdint>

class Hardware
{
 public:
  Hardware(const std::string& name);
  ~Hardware();

  // Get hardware name
  std::string getHardwareName(void) const;

  // Get video encoder name for GStreamer
  std::string getEncodingPipeline(void) const;

  // Get camera source name for GStreamer
  std::string getCameraSrc(void) const;

  // Does encoder take bitrate as kilobits instead of bits
  bool bitrateInKilobits(void) const;

 private:
  std::uint32_t hw;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
