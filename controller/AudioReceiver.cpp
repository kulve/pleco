/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "AudioReceiver.h"

#include <iostream>
#include <cstring>  // for memcpy

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <glib.h>

AudioReceiver::AudioReceiver(EventLoop& eventLoop)
  : eventLoop(eventLoop),
    pipeline(nullptr),
    source(nullptr),
    sink(nullptr),
    audioEnabled(false)
{
#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(nullptr);
  }
#endif

  std::cout << "AudioReceiver initialized" << std::endl;
}

AudioReceiver::~AudioReceiver()
{
  // Clean up
  std::cout << "Stopping audio playback" << std::endl;
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = nullptr;
  }
}

gboolean AudioReceiver::busCall(GstBus *bus, GstMessage *message, gpointer data)
{
  GError *error = nullptr;
  gchar *debug = nullptr;

  (void)bus;
  (void)data;

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
    gst_message_parse_error(message, &error, &debug);
    std::cerr << "Error: " << (error ? error->message : "unknown error") << std::endl;
    g_error_free(error);
    g_free(debug);
    break;

  case GST_MESSAGE_EOS:
    // end-of-stream
    std::cerr << "Warning: EOS" << std::endl;
    break;

  case GST_MESSAGE_WARNING:
    gst_message_parse_warning(message, &error, &debug);
    if (error != nullptr) {
      std::cerr << "Warning: " << error->message << std::endl;
      g_error_free(error);
    }
    if (debug != nullptr) {
      std::cout << "Debug: " << debug << std::endl;
      g_free(debug);
    }
    break;

  case GST_MESSAGE_ELEMENT:
  case GST_MESSAGE_STATE_CHANGED:
  case GST_MESSAGE_STREAM_STATUS:
    // Ignored
    break;

  default:
    // Unhandled message
    std::cerr << "Unhandled message: " << gst_message_type_get_name(GST_MESSAGE_TYPE(message)) << std::endl;
    break;
  }

  return true;
}

bool AudioReceiver::enableAudio(bool enable)
{
  GstBus *bus;
  GError *error = nullptr;

  if (!enable) {
    std::cerr << "disabling AudioReceiver not implemented" << std::endl;
    return false;
  }

  // Initialisation
  if (!gst_init_check(nullptr, nullptr, nullptr)) {
    std::cerr << "Failed to init GST" << std::endl;
    return false;
  }

  // Create receiving audio pipeline
  std::string pipelineString = "appsrc name=source ! "
                               "capsfilter caps=\"application/x-rtp,encoding-name=(string)X-GST-OPUS-DRAFT-SPITTKA-00, payload=(int)96\" ! "
                               "rtpopusdepay ! "
                               "opusdec ! "
                               "audioconvert ! "
                               "alsasink sync=false";

  std::cout << "Using pipeline: " << pipelineString << std::endl;

  // Create decoding audio pipeline
  pipeline = gst_parse_launch(pipelineString.c_str(), &error);
  if (!pipeline) {
    std::cerr << "Failed to parse pipeline: " << (error ? error->message : "unknown error") << std::endl;
    if (error) g_error_free(error);
    return false;
  }

  source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
  if (!source) {
    std::cerr << "Failed to get source" << std::endl;
    return false;
  }

  // Add a watch for messages on the pipeline's bus
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) busCall, this, nullptr);
  gst_object_unref(bus);

  // Start running
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
  audioEnabled = true;

  return true;
}

void AudioReceiver::consumeAudio(std::vector<std::uint8_t>* audio)
{
  std::cout << "AudioReceiver::consumeAudio" << std::endl;

  if (!audioEnabled || !source) {
    return;
  }

  GstBuffer *buffer = gst_buffer_new_and_alloc(audio->size());

  // Map the buffer for writing
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
    std::memcpy(map.data, audio->data(), audio->size());
    gst_buffer_unmap(buffer, &map);

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK) {
      std::cerr << "Error with gst_app_src_push_buffer" << std::endl;
    }
  } else {
    std::cerr << "Error with gst_buffer_map" << std::endl;
    gst_buffer_unref(buffer);
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
