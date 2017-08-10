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

#include "AudioReceiver.h"

#include <QDebug>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <glib.h>

#include <string.h>                          /* memcpy */

AudioReceiver::AudioReceiver():
  pipeline(NULL), source(NULL)
{

#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif
}



AudioReceiver::~AudioReceiver(void)
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

bool AudioReceiver::enableAudio(bool enable)
{
  GError *error = NULL;

  if (!enable) {
    qCritical("disabling AudioReceiver not implemented");
    return false;
  }

  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
    qCritical("Failed to init GST");
    return false;
  }

  // Create receiving audio pipeline
  QString pipelineString = "";
  pipelineString.append("appsrc name=source");
  pipelineString.append(" ! ");
  pipelineString.append("capsfilter caps=\"application/x-rtp,encoding-name=(string)X-GST-OPUS-DRAFT-SPITTKA-00, payload=(int)96\"");
  pipelineString.append(" ! ");
  pipelineString.append("rtpopusdepay");
  pipelineString.append(" ! ");
  pipelineString.append("opusdec");
  pipelineString.append(" ! ");
  pipelineString.append("audioconvert");
  pipelineString.append(" ! ");
  pipelineString.append("alsasink sync=false");

  qDebug() << "Using pipeline:" << pipelineString;

  // Create decoding audio pipeline
  pipeline = gst_parse_launch(pipelineString.toUtf8(), &error);
  if (!pipeline) {
    qCritical("Failed to parse pipeline: %s", error->message);
    g_error_free(error);
    return false;
  }

  source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
  if (!source) {
    qCritical("Failed to get source");
    return false;
  }

  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}



void AudioReceiver::consumeAudio(QByteArray *audio)
{
  qDebug() << "In" << __FUNCTION__;

  GstBuffer *buffer = gst_buffer_new_and_alloc(audio->length());

  // FIXME: zero copy?
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    memcpy(map.data, audio->data(), audio->length());
    gst_buffer_unmap(buffer, &map);

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK) {
      qWarning("Error with gst_app_src_push_buffer");
    }
  } else {
    qWarning("Error with gst_buffer_map");
  }

}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
