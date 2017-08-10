/*
 * Copyright 2017 Tuomas Kulve, <tuomas@kulve.fi>
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

#ifndef _VIDEORECEIVER_H
#define _VIDEORECEIVER_H

#include <QWidget>

#include <gst/gst.h>
#include <glib.h>

class VideoReceiver : public QWidget
{
  Q_OBJECT;

 public:
  VideoReceiver(QWidget *parent = 0);
  ~VideoReceiver(void);
  bool enableVideo(bool enable);
  quint16 getBufferFilled(void);

  public slots:
    void consumeVideo(QByteArray *Video);

  signals:
    void pos(double x_percent, double y_percent);
    void motorControlEvent(QKeyEvent *event);

  private:
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    static gboolean busCall(GstBus     *bus,
                            GstMessage *msg,
                            gpointer    data);

    WId xid;
    GstElement *pipeline;
    GstElement *source;
    GstElement *sink;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
