/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

 #include <iostream>
#include <cstring>  // for memcpy

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

#include "VideoReceiver.h"

// GStreamer callback for new sample from the Video Receiver pipeline
static GstFlowReturn newSampleCallback(GstElement* sink, VideoReceiver* self) {
  std::cout << "New sample received" << std::endl;

  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(structure, "width", &width);
  gst_structure_get_int(structure, "height", &height);

  // Map the buffer for reading
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    // Call the frame callback if set
    if (self->frameCallback) {
      self->frameCallback(map.data, width, height);
    }
    gst_buffer_unmap(buffer, &map);
  }

  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

VideoReceiver::VideoReceiver()
  : pipeline(nullptr),
    source(nullptr),
    sink(nullptr),
    positionCallback(nullptr),
    keyEventCallback(nullptr),
    width(0),
    height(0),
    videoEnabled(false)
{
#ifndef GLIB_VERSION_2_32
  // Must initialise GLib and it's threading system
  g_type_init();
  if (!g_thread_supported()) {
    g_thread_init(NULL);
  }
#endif

  std::cout << "VideoReceiver initialized" << std::endl;
}

VideoReceiver::~VideoReceiver()
{
  // Clean up
  if (pipeline) {
    std::cout << "Stopping video playback" << std::endl;
    // Proper state change sequence to ensure proper cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Wait for the state change to complete
    GstState state;
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);

    // Now it's safe to unref
    gst_object_unref(GST_OBJECT(pipeline));
    pipeline = nullptr;
    source = nullptr; // These are owned by the pipeline
    sink = nullptr;
  }
}

gboolean VideoReceiver::busCall(GstBus *bus, GstMessage *message, gpointer data)
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
    // Ignored;
    break;

  default:
    // Unhandled message
    std::cerr << "Unhandled message: " << gst_message_type_get_name(GST_MESSAGE_TYPE(message)) << std::endl;
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
    std::cerr << "disabling VideoReceiver" << std::endl;
    return false;
  }

  // Initialisation
  if (!gst_init_check(nullptr, nullptr, nullptr)) {
    std::cerr << "Failed to init GST" << std::endl;
    return false;
  }

  // Create receiving video pipeline
  pipeline = gst_pipeline_new("videopipeline");
  source = gst_element_factory_make("appsrc", "source");

#if USE_JITTER_BUFFER == 1
  jitterbuffer = gst_element_factory_make("rtpjitterbuffer", "jitterbuffer");
#endif
  rtpdepay = gst_element_factory_make("rtph264depay", "rtpdepay");
  decoder = gst_element_factory_make("avdec_h264", "decoder");
  GstElement *convert = gst_element_factory_make("videoconvert", "converter");

  sink = gst_element_factory_make("appsink", "sink");

  // Configure the appsink to emit signals when new frames arrive
  g_object_set(G_OBJECT(sink), "emit-signals", TRUE, nullptr);
  g_object_set(G_OBJECT(sink), "sync", FALSE, nullptr);

  // Set the caps for the appsink
  GstCaps* sinkCaps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "RGBA",
                                        nullptr);
  gst_app_sink_set_caps(GST_APP_SINK(sink), sinkCaps);
  gst_caps_unref(sinkCaps);

  // Connect to the new-sample signal
  g_signal_connect(sink, "new-sample", G_CALLBACK(newSampleCallback), this);

  GstElement *parse = gst_element_factory_make("h264parse", "h264parse");

  g_object_set(G_OBJECT(sink), "sync", false, nullptr);
  g_object_set(G_OBJECT(source), "do-timestamp", true, nullptr);
  g_object_set(G_OBJECT(source), "is-live", true, nullptr);
  g_object_set(G_OBJECT(source), "stream-type", 0, nullptr);
  g_object_set(G_OBJECT(source), "max-bytes", 10000, nullptr);
  g_object_set(G_OBJECT(source), "format", 3, nullptr);

#if USE_JITTER_BUFFER == 1
  g_object_set(G_OBJECT(jitterbuffer), "latency", 100, nullptr);
  g_object_set(G_OBJECT(jitterbuffer), "do-lost", true, nullptr);
  g_object_set(G_OBJECT(jitterbuffer), "drop-on-latency", 1, nullptr);
#endif

  // Set the caps for appsrc
  caps = gst_caps_new_simple("application/x-rtp",
                             "media", G_TYPE_STRING, "video",
                             "clock-rate", G_TYPE_INT, 90000,
                             "encoding-name", G_TYPE_STRING, "H264",
                             "payload", G_TYPE_INT, 96,
                             nullptr);
  gst_app_src_set_caps(GST_APP_SRC(source), caps);
  gst_caps_unref(caps);

  gst_bin_add_many(GST_BIN(pipeline),
#if USE_JITTER_BUFFER == 1
                   jitterbuffer,
#endif
                   source, rtpdepay, parse, decoder, convert, sink, nullptr);

  // Link
  if (!gst_element_link_many(source,
#if USE_JITTER_BUFFER == 1
                             jitterbuffer,
#endif
                             rtpdepay, parse, decoder, convert, sink, nullptr)) {
    std::cerr << "Failed to link elements!" << std::endl;
    return false;
  }

  // Add a watch for new messages on our pipeline's message bus
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_set_sync_handler(bus, (GstBusSyncHandler) busCall, this, nullptr);
  gst_object_unref(bus);

  // Start running
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  videoEnabled = true;
  return true;
}

void VideoReceiver::setPositionCallback(PositionCallback callback)
{
  positionCallback = callback;
}

void VideoReceiver::setKeyEventCallback(KeyEventCallback callback)
{
  keyEventCallback = callback;
}

void VideoReceiver::consumeVideo(std::vector<std::uint8_t>* video, std::uint8_t index)
{
  std::cout << "Received video, index: " << static_cast<int>(index) << std::endl;

  // TODO: handle other video streams as well
  if (index != 0) {
    std::cerr << "Unknown video index: " << static_cast<int>(index) << std::endl;
    delete video;
    return;
  }

  if (!videoEnabled || !source) {
    std::cerr << "VideoReceiver not enabled or source not set" << std::endl;
    delete video;
    return;
  }

  GstBuffer *buffer = gst_buffer_new_and_alloc(video->size());

  // Map the buffer for writing
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
    std::memcpy(map.data, video->data(), video->size());
    gst_buffer_unmap(buffer, &map);

    if (gst_app_src_push_buffer(GST_APP_SRC(source), buffer) != GST_FLOW_OK) {
      std::cerr << "Error with gst_app_src_push_buffer" << std::endl;
    }
  } else {
    std::cerr << "Error with gst_buffer_map" << std::endl;
    gst_buffer_unref(buffer);
  }

  // Delete the video data as we've copied it into the GStreamer buffer
  delete video;
}

void VideoReceiver::handleMouseMove(SDL_MouseMotionEvent* event)
{
  if (!positionCallback) return;

  double x_percent = event->x / static_cast<double>(width);

  // reverse y
  double y_percent = 1.0 - (event->y / static_cast<double>(height));

  // clamp to 0-1
  x_percent = std::max(0.0, std::min(x_percent, 1.0));
  y_percent = std::max(0.0, std::min(y_percent, 1.0));

  positionCallback(x_percent, y_percent);
}

void VideoReceiver::handleKeyPress(SDL_KeyboardEvent* event)
{
  if (keyEventCallback) {
    keyEventCallback(event);
  }
}

void VideoReceiver::handleKeyRelease(SDL_KeyboardEvent* event)
{
  if (keyEventCallback) {
    keyEventCallback(event);
  }
}

SDL_Texture* VideoReceiver::createTextureFromFrame(SDL_Renderer* /*renderer*/)
{
  // This would need to be implemented to get a texture from GStreamer
  // For a complete implementation, you would need to:
  // 1. Add a video sink that can export frames (like appsink)
  // 2. Get the raw frame data
  // 3. Create an SDL texture from the frame data

  // Placeholder - would need actual implementation
  return nullptr;
}

std::uint16_t VideoReceiver::getBufferFilled()
{
  GstElement *jitterbuffer;
  gint percent = 0;

#if USE_JITTER_BUFFER == 0
  return 0;
#endif

  if (!pipeline) {
    return 0;
  }

  jitterbuffer = gst_bin_get_by_name(GST_BIN(pipeline), "jitterbuffer");

  if (!jitterbuffer) {
    std::cerr << "Failed to get jitterbuffer" << std::endl;
    return 0;
  }

  g_object_get(G_OBJECT(jitterbuffer),
               "percent", &percent,
               nullptr);

  std::cout << "Buffer filled: " << percent << "%" << std::endl;

  // Unref the element we got from get_by_name
  gst_object_unref(jitterbuffer);

  return static_cast<std::uint16_t>(percent);
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
