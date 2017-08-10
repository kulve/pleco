/*
 * Copyright 2012-2017 Tuomas Kulve, <tuomas@kulve.fi>
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

#include "AudioSender.h"

#include <QObject>
#include <QDebug>
#include <QProcess>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

AudioSender::AudioSender(Hardware *hardware):
  QObject(), pipeline(NULL), hardware(hardware), encoder(NULL)
{

#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and its threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

}



AudioSender::~AudioSender()
{ 

  // Clean up
  qDebug() << "Stopping audio encoding";
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  qDebug() << "Deleting pipeline";
  if (pipeline) {
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = NULL;
  }

}



bool AudioSender::enableSending(bool enable)
{
  GstElement *sink;
  GError *error = NULL;

  qDebug() << "In" << __FUNCTION__ << ", Enable:" << enable;

  // Disable audio sending
  if (enable == false) {
    qDebug() << "Stopping audio encoding";
    if (pipeline) {
      gst_element_set_state(pipeline, GST_STATE_NULL);
    }

    qDebug() << "Deleting pipeline";
    if (pipeline) {
      gst_object_unref(GST_OBJECT(pipeline));
      pipeline = NULL;
    }
    encoder = NULL;

    return true;
  }

  if (pipeline) {
    // Do nothing as the pipeline has already been created and is
    // probably running
    qCritical("Pipeline exists already, doing nothing");
    return true;
  }

  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
    qCritical("Failed to init GST");
    return false;
  }

  if (!hardware) {
    qCritical("No hardware plugin");
    return false;
  }

  QString pipelineString = "";
  pipelineString.append("alsasrc name=source");
  pipelineString.append(" ! ");
  pipelineString.append("capsfilter caps=\"audio/x-raw,channels=1\"");
  pipelineString.append(" ! ");
  pipelineString.append("opusenc");
  pipelineString.append(" ! ");
  pipelineString.append("rtpopuspay mtu=500");
  pipelineString.append(" ! ");
  pipelineString.append("appsink name=sink sync=false max-buffers=1 drop=true");

  qDebug() << "Using pipeline:" << pipelineString;

  // Create encoding audio pipeline
  pipeline = gst_parse_launch(pipelineString.toUtf8(), &error);
  if (!pipeline) {
    qCritical("Failed to parse pipeline: %s", error->message);
    g_error_free(error);
    return false;
  }

  QByteArray env_alsa_device = qgetenv("PLECO_SLAVE_ALSA_DEVICE");
  if (!env_alsa_device.isNull()) {

    GstElement *source;
    source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (!source) {
      qCritical("Failed to get source");
      return false;
    }

    char *device = env_alsa_device.data();
    g_object_set(G_OBJECT(source), "device", device, NULL);
  }

  sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (!sink) {
    qCritical("Failed to get sink");
    return false;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks appSinkCallbacks;
  appSinkCallbacks.eos             = NULL;
  appSinkCallbacks.new_preroll     = NULL;
  appSinkCallbacks.new_sample      = &newBufferCB;

  gst_app_sink_set_callbacks(GST_APP_SINK(sink), &appSinkCallbacks, this, NULL);

  // Start running 
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}



void AudioSender::emitAudio(QByteArray *data)
{
  qDebug() << "In" << __FUNCTION__;

  emit(audio(data));
}



GstFlowReturn AudioSender::newBufferCB(GstAppSink *sink, gpointer user_data)
{
  qDebug() << "In" << __FUNCTION__;

  AudioSender *vs = static_cast<AudioSender *>(user_data);

  // Get new audio sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    qWarning("%s: Failed to get new sample", __FUNCTION__);
    return GST_FLOW_OK;
  }
  
  // FIXME: zero copy?
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  QByteArray *data = NULL;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    // Copy the data to QByteArray
    data = new QByteArray((char *)map.data, map.size);
    vs->emitAudio(data);
    gst_buffer_unmap(buffer, &map);
  } else {
    qWarning("Error with gst_buffer_map");
  }
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}



/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
