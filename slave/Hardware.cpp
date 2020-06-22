/*
 * Copyright 2013-2020 Tuomas Kulve, <tuomas@kulve.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Hardware.h"

#include <QDebug>

struct hardwareInfo {
  QString name;
  QString videoEncoder;
  QString cameraSrc;
  bool    bitrateInKilobits;
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


Hardware::Hardware(QString name)
{
  for (uint i = 0; i < sizeof(hardwareList); ++i) {
    if (hardwareList[i].name == name) {
      hw = i;
      qDebug() << "in" << __FUNCTION__ << ", selected: " << hardwareList[hw].name;
      return;
    }
  }
}



Hardware::~Hardware(void)
{
  // Nothing here
}


QString Hardware::getHardwareName(void) const
{
  return hardwareList[hw].name;
}

QString Hardware::getEncodingPipeline(void) const
{
  return hardwareList[hw].videoEncoder;
}

QString Hardware::getCameraSrc(void) const
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
