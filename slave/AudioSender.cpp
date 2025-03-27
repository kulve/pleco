/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "AudioSender.h"

#include <iostream>
#include <string>
#include <cstdlib>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

AudioSender::AudioSender(Hardware *hardware):
  pipeline(nullptr), hardware(hardware), encoder(nullptr)
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
  std::cout << "Stopping audio encoding" << std::endl;
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
  }

  std::cout << "Deleting pipeline" << std::endl;
  if (pipeline) {
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = nullptr;
  }
}

bool AudioSender::enableSending(bool enable)
{
  GstElement *sink;
  GError *error = nullptr;

  std::cout << "In " << __FUNCTION__ << ", Enable: " << (enable ? "true" : "false") << std::endl;

  // Disable audio sending
  if (!enable) {
    std::cout << "Stopping audio encoding" << std::endl;
    if (pipeline) {
      gst_element_set_state(pipeline, GST_STATE_NULL);
    }

    std::cout << "Deleting pipeline" << std::endl;
    if (pipeline) {
      gst_object_unref(GST_OBJECT(pipeline));
      pipeline = nullptr;
    }
    encoder = nullptr;

    return true;
  }

  if (pipeline) {
    // Do nothing as the pipeline has already been created and is
    // probably running
    std::cerr << "Pipeline exists already, doing nothing" << std::endl;
    return true;
  }

  // Initialisation. We don't pass command line arguments here
  if (!gst_init_check(NULL, NULL, NULL)) {
    std::cerr << "Failed to init GST" << std::endl;
    return false;
  }

  if (!hardware) {
    std::cerr << "No hardware plugin" << std::endl;
    return false;
  }

  std::string pipelineString = "alsasrc name=source";
  pipelineString += " ! ";
  pipelineString += "capsfilter caps=\"audio/x-raw,channels=1\"";
  pipelineString += " ! ";
  pipelineString += "opusenc";
  pipelineString += " ! ";
  pipelineString += "rtpopuspay mtu=500";
  pipelineString += " ! ";
  pipelineString += "appsink name=sink sync=false max-buffers=1 drop=true";

  std::cout << "Using pipeline: " << pipelineString << std::endl;

  // Create encoding audio pipeline
  pipeline = gst_parse_launch(pipelineString.c_str(), &error);
  if (!pipeline) {
    std::cerr << "Failed to parse pipeline: " << error->message << std::endl;
    g_error_free(error);
    return false;
  }

  char* env_alsa_device = std::getenv("PLECO_SLAVE_ALSA_DEVICE");
  if (env_alsa_device) {
    GstElement *source;
    source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if (!source) {
      std::cerr << "Failed to get source" << std::endl;
      return false;
    }

    g_object_set(G_OBJECT(source), "device", env_alsa_device, NULL);
  }

  sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (!sink) {
    std::cerr << "Failed to get sink" << std::endl;
    return false;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks appSinkCallbacks;
  appSinkCallbacks.eos = NULL;
  appSinkCallbacks.new_preroll = NULL;
  appSinkCallbacks.new_sample = &newBufferCB;

  gst_app_sink_set_callbacks(GST_APP_SINK(sink), &appSinkCallbacks, this, NULL);

  // Start running
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  return true;
}

void AudioSender::emitAudio(std::vector<std::uint8_t> *data)
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  if (audioCallback) {
    audioCallback(data);
  }
}

void AudioSender::setAudioCallback(AudioCallback callback)
{
  audioCallback = callback;
}

GstFlowReturn AudioSender::newBufferCB(GstAppSink *sink, gpointer user_data)
{
  std::cout << "In " << __FUNCTION__ << std::endl;

  AudioSender *as = static_cast<AudioSender *>(user_data);

  // Get new audio sample
  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (sample == NULL) {
    std::cerr << __FUNCTION__ << ": Failed to get new sample" << std::endl;
    return GST_FLOW_OK;
  }

  // FIXME: zero copy?
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstMapInfo map;
  std::vector<std::uint8_t> *data = NULL;

  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    // Copy the data to std::vector
    data = new std::vector<std::uint8_t>(map.data, map.data + map.size);
    as->emitAudio(data);
    gst_buffer_unmap(buffer, &map);
  } else {
    std::cerr << "Error with gst_buffer_map" << std::endl;
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
