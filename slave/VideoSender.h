/*
 * Copyright 2012 Tuomas Kulve, <tuomas@kulve.fi>
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

#ifndef _VIDEOSENDER_H
#define _VIDEOSENDER_H

#include "Hardware.h"

#include <QObject>
#include <QProcess>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

class VideoSender : public QObject
{
  Q_OBJECT;

 public:
  VideoSender(Hardware *hardware, quint8 index);
  ~VideoSender();
  bool enableSending(bool enable);
  void setVideoSource(int index);
  void setVideoQuality(quint16 quality);

 signals:
  void video(QByteArray *video, quint8 index);

  private slots:
    void ODreadyRead();
    void ODfinished(int exitCode, QProcess::ExitStatus exitStatus);

 private:
  void setBitrate(int bitrate);
  void emitVideo(QByteArray *data);
  void launchObjectDetection();
  static GstFlowReturn newBufferCB(GstAppSink *sink, gpointer user_data);
  static GstFlowReturn newBufferOBCB(GstAppSink *sink, gpointer user_data);

  GstElement *pipeline;
  QString videoSource;

  Hardware *hardware;

  GstElement *encoder;

  int bitrate;
  quint16 quality;
  quint8 index;
  quint8 ODdata[6];
  QProcess *ODprocess;
  bool ODprocessReady;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
