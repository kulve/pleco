/*
 * Copyright 2013-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Hardware.h"

#include <iostream>
#include <string>
#include <cstdint>

struct hardwareInfo {
  std::string name;
  std::string videoEncoder;
  std::string cameraSrc;
  bool bitrateInKilobits;
};

static const struct hardwareInfo hardwareList[] = {
  {
    "gumstix_overo",
    "videoconvert ! dsph264enc name=encoder",
    "v4l2src",
    false
  },
  {
    "generic_x86",
    "x264enc name=encoder",
    "v4l2src",
    true
  },
  {
    "tegra3",
    "nvvidconv ! capsfilter caps=video/x-nvrm-yuv ! nv_omx_h264enc name=encoder",
    "v4l2src",
    false
  },
  {
    "tegrak1",
    "omxh264enc name=encoder",
    "v4l2src",
    false
  },
  {
    "tegrax1",
    "omxh264enc name=encoder",
    "v4l2src",
    false
  },
  {
    "tegrax2",
    "omxh264enc name=encoder",
    "v4l2src",
    false
  },
  {
    "tegra_nano",
    "omxh264enc name=encoder",
    "nvarguscamerasrc",
    false
  },
};

Hardware::Hardware(const std::string& name)
  : hw(0) // Default to first hardware in the list
{
  for (std::uint32_t i = 0; i < sizeof(hardwareList) / sizeof(hardwareList[0]); ++i) {
    if (hardwareList[i].name == name) {
      hw = i;
      std::cout << "in " << __FUNCTION__ << ", selected: " << hardwareList[hw].name << std::endl;
      return;
    }
  }
}

Hardware::~Hardware(void)
{
  // Nothing here
}

std::string Hardware::getHardwareName(void) const
{
  return hardwareList[hw].name;
}

std::string Hardware::getEncodingPipeline(void) const
{
  return hardwareList[hw].videoEncoder;
}

std::string Hardware::getCameraSrc(void) const
{
  return hardwareList[hw].cameraSrc;
}

bool Hardware::bitrateInKilobits(void) const
{
  return hardwareList[hw].bitrateInKilobits;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
