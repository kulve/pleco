/*
 * Copyright 2020 Tuomas Kulve, <tuomas@kulve.fi>
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

#ifndef _HMD_H
#define _HMD_H

#include <QString>
#include <QLocalSocket>
#include <openhmd.h>

class HMD : public QObject
{
  Q_OBJECT;

 public:
  HMD();
  ~HMD();
  bool init();
  bool reset_orientation();
  bool get_modelview_matrix_left(float matrix[16]);
  bool get_modelview_matrix_right(float matrix[16]);
  bool get_projection_matrix_left(float matrix[16]);
  bool get_projection_matrix_right(float matrix[16]);
  bool get_distortion_vertex_src(const char **vertex);
  bool get_distortion_fragment_src(const char **fragment);

 signals:
  void axisChanged(int axis, quint16 value);
  void buttonChanged(int button, quint16 value);

 private slots:
  //void readPendingInputData();

 private:
  ohmd_context* ctx;
  ohmd_device* hmd;
  int hmd_w;
  int hmd_h;
  float ipd;
  float viewport_scale[2];
  float distortion_coeffs[4];
  float aberr_scale[3];
  float sep;
  float left_lens_center[2];
  float right_lens_center[2];
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
