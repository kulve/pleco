/*
 * Copyright 2013 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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
};

static const struct hardwareInfo hardwareList[] = {
  {
	"gumstix_overo",
	"dsph263enc"
  },
  {
	"generic_x86",
	"ffenc_h263"
  }
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


QString Hardware::getVideoEncoderName(void) const
{
  return hardwareList[hw].videoEncoder;
}

