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

#include "VideoReceiver.h"

#include <QWidget>
#include <QDebug>
#include <QMouseEvent>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/app/gstappsrc.h>
#include <glib.h>

#include <string.h>                          /* memcpy */

VideoReceiver::VideoReceiver(QWidget *parent):
  QWidget(parent), xid(0), pipeline(NULL), source(NULL), sink(NULL)
{

#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

  xid = winId();
  qDebug() << __FUNCTION__ << "xid:" << xid;

  setFocusPolicy(Qt::StrongFocus);
}



VideoReceiver::~VideoReceiver(void)
{

  // Clean up
  qDebug() << "Stopping playback";
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  qDebug() << "Deleting pipeline";
  if (pipeline) {
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = NULL;
  }

}

gboolean VideoReceiver::busCall(GstBus     *,
                                GstMessage *message,
                                gpointer    data)
{
  GError *error = NULL;
  gchar *debug = NULL;

  (void)data;

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
    gst_message_parse_error(message, &error, &debug);
    qWarning("Error: %s", error->message);
    g_error_free(error);
    g_free(debug);

    break;
  case GST_MESSAGE_EOS:
    // end-of-stream
    // g_main_loop_quit(loop);
    qWarning("%s: Warning: EOS", __FUNCTION__);
    break;
  case GST_MESSAGE_WARNING:
    gst_message_parse_warning(message, &error, &debug);
    if (error != NULL) {
      qWarning("Warning: %s", error->message);
      g_error_free(error);
    }
    if (debug != NULL) {
      qDebug("Debug: %s", debug);
      g_free(debug);
    }
    break;
  case GST_MESSAGE_ELEMENT:
  case GST_MESSAGE_STATE_CHANGED:
  case GST_MESSAGE_STREAM_STATUS:
    // Ignored;
    break;
  default:
    // Unhandled message
    qWarning("Unhandled message: %s", gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
    break;
  }

  return true;
}



bool VideoReceiver::enableVideo(bool enable)
{
#define USE_JITTER_BUFFER 0
#if USE_JITTER_BUFFER == 1
  GstElement *jitterbuffer;
#endif
  GstElement *rtpdepay, *decoder;
  GstBus *bus;
  GstCaps *caps;

  if (!enable) {
    qCritical("disabling VideoReceiver not implemented");
    return false;
  }


  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
    qCritical("Failed to init GST");
    return false;
  }

  // Create receiving video pipeline 

  pipeline = gst_pipeline_new("videopipeline");

  source        = gst_element_factory_make("appsrc", "source");

#if USE_JITTER_BUFFER == 1
  jitterbuffer  = gst_element_factory_make("rtpjitterbuffer", "jitterbuffer");
#endif
  rtpdepay      = gst_element_factory_make("rtph264depay", "rtpdepay");
  decoder       = gst_element_factory_make("avdec_h264", "decoder");
  sink          = gst_element_factory_make("xvimagesink", "sink");

  GstElement *parse         = gst_element_factory_make("h264parse", "h264parse");

  g_object_set(G_OBJECT(sink), "sync", false, NULL);
  // Drop after max-lateness (default 20ms, in ns)
  //g_object_set(G_OBJECT(sink), "max-lateness", -1, NULL);
  g_object_set(G_OBJECT(source), "do-timestamp", true, NULL);
  // Set the stream to act "live stream"
  g_object_set(G_OBJECT(source), "is-live", true, NULL);
  // Set the stream type to "stream"
  g_object_set(G_OBJECT(source), "stream-type", 0, NULL);
  // Limit internal queue to 10k (default 200k)
  g_object_set(G_OBJECT(source), "max-bytes", 10000, NULL);
  // Set format to GST_FORMAT_TIME
  g_object_set(G_OBJECT(source), "format", 3, NULL);

  // Tune jitter buffer
#if USE_JITTER_BUFFER == 1
  g_object_set(G_OBJECT(jitterbuffer), "latency", 100, NULL);
  g_object_set(G_OBJECT(jitterbuffer), "do-lost", true, NULL);
  g_object_set(G_OBJECT(jitterbuffer), "drop-on-latency", 1, NULL);
#endif

  // Set the caps for appsrc
  caps = gst_caps_new_simple("application/x-rtp",
                             "media", G_TYPE_STRING, "video",
                             "clock-rate", G_TYPE_INT, 90000,
                             "encoding-name", G_TYPE_STRING, "H264",
                             "payload", G_TYPE_INT, 96,
                             NULL);
  gst_app_src_set_caps(GST_APP_SRC(source), caps);
  gst_caps_unref (caps);

  gst_bin_add_many(GST_BIN(pipeline), 
#if USE_JITTER_BUFFER == 1
                   jitterbuffer,
#endif
                   source, rtpdepay, parse, decoder, sink, NULL);

  // Link 
  if (!gst_element_link_many(source,
#if USE_JITTER_BUFFER == 1
                             jitterbuffer,
#endif
                             rtpdepay, decoder, sink, NULL)) {
    qCritical("Failed to link elements!");
    return false;
  }

  // Add a watch for new messages on our pipeline's message bus
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) busCall, this,  NULL);
  gst_object_unref(bus);
  bus = NULL;

  gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(sink), xid);

  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}



void VideoReceiver::consumeVideo(QByteArray *video, quint8 index)
{
  qDebug() << "In" << __FUNCTION__ << ", index: " << index;

  // TODO: handle other video streams as well
  if (index != 0) {
    return;
  }

  GstBuffer *buffer = gst_buffer_new_and_alloc(video->length());

  // FIXME: zero copy?
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    memcpy(map.data, video->data(), video->length());
    gst_buffer_unmap(buffer, &map);

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK) {
      qWarning("Error with gst_app_src_push_buffer");
    }
  } else {
    qWarning("Error with gst_buffer_map");
  }

}



void VideoReceiver::mouseMoveEvent(QMouseEvent *event)
{

  double x_percent = event->x() / (double)(this->width());

  // reverse y
  double y_percent = 1 - (event->y() / (double)(this->height()));

  //clamp to 0-1
  if (x_percent < 0) x_percent = 0;
  if (y_percent < 0) y_percent = 0;
  if (x_percent > 1) x_percent = 1;
  if (y_percent > 1) y_percent = 1;

  //qDebug() << "In" << __FUNCTION__ << ", X Y:" << x_percent << y_percent;

  emit(pos(x_percent, y_percent));
}



void VideoReceiver::keyPressEvent(QKeyEvent *event)
{
  emit(motorControlEvent(event));
}



void VideoReceiver::keyReleaseEvent(QKeyEvent *event)
{
  emit(motorControlEvent(event));
}


quint16 VideoReceiver::getBufferFilled(void)
{
  GstElement *jitterbuffer;
  gint percent;

#if USE_JITTER_BUFFER == 0
  return 0;
#endif

  if (!pipeline) {
    return 0;
  }

  jitterbuffer = gst_bin_get_by_name(GST_BIN(pipeline), "jitterbuffer");

  if (!jitterbuffer) {
    qWarning("Failed to get jitterbuffer");
    return 0;
  }

  g_object_get(G_OBJECT(jitterbuffer),
               "percent", &percent,
               NULL);
  qDebug() << "In" << __FUNCTION__ << "percent:" << percent;
  return percent;
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
