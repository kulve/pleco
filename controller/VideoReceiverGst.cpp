/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */
#include "VideoReceiverGst.h"

#include <iostream>
#include <cstring>  // for memcpy
#include <chrono>   // for timestamps

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

VideoReceiverGst::VideoReceiverGst()
  : pipeline(nullptr),
    source(nullptr),
    sink(nullptr),
    videoEnabled(false),
    initialized(false)
{
  std::cout << "VideoReceiverGst created" << std::endl;
}

VideoReceiverGst::~VideoReceiverGst()
{
  deinit();
}

bool VideoReceiverGst::init()
{
  if (initialized) {
    return true;
  }

  // Initialization
  if (!gst_init_check(nullptr, nullptr, nullptr)) {
    std::cerr << "Failed to init GStreamer" << std::endl;
    return false;
  }
  // Create or update the pipeline if needed
  if (!pipeline) {
    if (!createPipeline()) {
      return false;
    }
  }

  initialized = true;
  std::cout << "VideoReceiverGst initialized" << std::endl;
  return true;
}

void VideoReceiverGst::deinit()
{
  // Clean up pipeline and resources
  if (pipeline) {
    std::cout << "Stopping GStreamer pipeline" << std::endl;
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

  // Clean up stored frames
  {
    std::lock_guard<std::mutex> lock(frameMutex);
    decodedFrames.clear();
  }

  videoEnabled = false;
  std::cout << "VideoReceiverGst deinitialized" << std::endl;
}

gboolean VideoReceiverGst::busCall(GstBus *bus, GstMessage *message, gpointer data)
{
  GError *error = nullptr;
  gchar *debug = nullptr;

  (void)bus;
  VideoReceiverGst* self = static_cast<VideoReceiverGst*>(data);
  if (!self) {
    std::cerr << "Invalid VideoReceiverGst pointer in bus call" << std::endl;
    return TRUE;
  }

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_ERROR:
    gst_message_parse_error(message, &error, &debug);
    std::cerr << "Error: " << (error ? error->message : "unknown error") << std::endl;
    if (debug) {
      std::cerr << "Debug info: " << debug << std::endl;
    }
    g_error_free(error);
    g_free(debug);
    break;

  case GST_MESSAGE_EOS:
    // end-of-stream
    std::cerr << "End of stream reached" << std::endl;
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
    // Silently ignored
    break;

  default:
    // Unhandled message
    std::cerr << "Unhandled message: " << gst_message_type_get_name(GST_MESSAGE_TYPE(message)) << std::endl;
    break;
  }

  return TRUE; // Continue processing
}

GstFlowReturn VideoReceiverGst::onNewSample(GstElement* sink, gpointer userData)
{
  VideoReceiverGst* self = static_cast<VideoReceiverGst*>(userData);
  if (!self) {
    std::cerr << "Invalid VideoReceiverGst pointer in sample callback" << std::endl;
    return GST_FLOW_ERROR;
  }

  GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
  if (!sample) {
    std::cerr << "Failed to get sample from sink" << std::endl;
    return GST_FLOW_ERROR;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  if (!buffer) {
    std::cerr << "Sample contains no buffer" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  GstCaps* caps = gst_sample_get_caps(sample);
  if (!caps) {
    std::cerr << "Sample has no caps" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  GstStructure* structure = gst_caps_get_structure(caps, 0);
  if (!structure) {
    std::cerr << "Caps has no structure" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  int width, height;
  if (!gst_structure_get_int(structure, "width", &width) ||
      !gst_structure_get_int(structure, "height", &height)) {
    std::cerr << "Failed to get dimensions from caps" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  // Get stride if available
  int stride = width * 4; // Default stride for RGBA
  gst_structure_get_int(structure, "stride", &stride);

  // Get timestamp of the frame
  uint64_t timestamp = GST_BUFFER_PTS(buffer);
  if (timestamp == GST_CLOCK_TIME_NONE) {
    // Use system time if no buffer timestamp
    auto now = std::chrono::high_resolution_clock::now();
    timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
  } else {
    // Convert from GStreamer time (nanoseconds) to microseconds
    timestamp = timestamp / 1000;
  }

  // Map the buffer for reading
  GstMapInfo map;
  if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    std::cerr << "Failed to map buffer" << std::endl;
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  // Store the decoded frame for later retrieval
  self->storeDecodedFrame(map.data, width, height, stride, timestamp);

  // Clean up
  gst_buffer_unmap(buffer, &map);
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}

void VideoReceiverGst::storeDecodedFrame(const void* data, int width, int height, int stride, int64_t timestamp)
{
    std::lock_guard<std::mutex> lock(frameMutex);

    // Create a new FrameData object for storing the frame
    FrameData newFrame;
    newFrame.width = width;
    newFrame.height = height;
    newFrame.stride = stride;
    newFrame.timestamp = timestamp;

    // Create a new buffer to hold the pixel data (must be managed by decodedFrames)
    size_t dataSize = height * stride;
    std::vector<std::uint8_t> frameBuffer(dataSize);
    memcpy(frameBuffer.data(), data, dataSize);

    // Add new frame to our storage (newest first)
    // Store the pixel data buffer in the deque so it persists
    decodedFrames.push_front(std::move(newFrame));

    // Make a copy of the buffer for this frame
    decodedFrames.front().pixelBuffer = std::move(frameBuffer);

    // Point the pixels pointer to our managed buffer
    decodedFrames.front().pixels = decodedFrames.front().pixelBuffer.data();

    // Clean up old frames to avoid using too much memory
    cleanupOldFrames();
}

void VideoReceiverGst::cleanupOldFrames()
{
  // Keep only the last 5 frames at most
  const size_t MAX_FRAMES = 5;

  if (decodedFrames.size() > MAX_FRAMES) {
    decodedFrames.resize(MAX_FRAMES);
  }
}

void VideoReceiverGst::consumeBitStream(std::vector<std::uint8_t>* video)
{
  if (!video) {
    std::cerr << "Received null video data" << std::endl;
    return;
  }

  std::cout << "Received video data: " << video->size() << " bytes" << std::endl;

  if (!videoEnabled || !source) {
    std::cerr << "VideoReceiver not enabled or source not set" << std::endl;
    delete video;
    return;
  }

  GstBuffer* buffer = gst_buffer_new_and_alloc(video->size());
  if (!buffer) {
    std::cerr << "Failed to allocate GstBuffer" << std::endl;
    delete video;
    return;
  }

  // Map the buffer for writing
  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
    std::memcpy(map.data, video->data(), video->size());
    gst_buffer_unmap(buffer, &map);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(source), buffer);
    if (ret != GST_FLOW_OK) {
      std::cerr << "Error pushing buffer: " << gst_flow_get_name(ret) << std::endl;
    }
  } else {
    std::cerr << "Error mapping buffer" << std::endl;
    gst_buffer_unref(buffer);
  }

  // Delete the video data as we've copied it into the GStreamer buffer
  delete video;
}

bool VideoReceiverGst::createPipeline()
{
  if (pipeline) {
    return true; // Already created
  }

  std::cout << "Creating GStreamer pipeline" << std::endl;

  // Create receiving video pipeline
  pipeline = gst_pipeline_new("videopipeline");
  if (!pipeline) {
    std::cerr << "Failed to create pipeline" << std::endl;
    return false;
  }

  source = gst_element_factory_make("appsrc", "source");
  if (!source) {
    std::cerr << "Failed to create appsrc element" << std::endl;
    gst_object_unref(pipeline);
    pipeline = nullptr;
    return false;
  }

  GstElement* rtpdepay = gst_element_factory_make("rtph264depay", "rtpdepay");
  GstElement* parse = gst_element_factory_make("h264parse", "h264parse");
  GstElement* decoder = gst_element_factory_make("avdec_h264", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvert", "converter");
  sink = gst_element_factory_make("appsink", "sink");

  if (!rtpdepay || !parse || !decoder || !convert || !sink) {
    std::cerr << "Failed to create pipeline elements" << std::endl;
    gst_object_unref(pipeline);
    pipeline = nullptr;
    return false;
  }

  // Configure the appsink
  g_object_set(G_OBJECT(sink), "emit-signals", TRUE, nullptr);
  g_object_set(G_OBJECT(sink), "sync", FALSE, nullptr);

  // Set the caps for the appsink
  GstCaps* sinkCaps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "RGBA",
                                        nullptr);
  gst_app_sink_set_caps(GST_APP_SINK(sink), sinkCaps);
  gst_caps_unref(sinkCaps);

  // Connect to the new-sample signal
  g_signal_connect(sink, "new-sample", G_CALLBACK(onNewSample), this);

  // Configure the appsrc
  g_object_set(G_OBJECT(source), "do-timestamp", TRUE, nullptr);
  g_object_set(G_OBJECT(source), "is-live", TRUE, nullptr);
  g_object_set(G_OBJECT(source), "stream-type", 0, nullptr); // 0 = GST_APP_STREAM_TYPE_STREAM
  g_object_set(G_OBJECT(source), "format", 3, nullptr);      // 3 = GST_FORMAT_TIME

  // Set the caps for appsrc
  GstCaps* srcCaps = gst_caps_new_simple("application/x-rtp",
                             "media", G_TYPE_STRING, "video",
                             "clock-rate", G_TYPE_INT, 90000,
                             "encoding-name", G_TYPE_STRING, "H264",
                             "payload", G_TYPE_INT, 96,
                             nullptr);
  gst_app_src_set_caps(GST_APP_SRC(source), srcCaps);
  gst_caps_unref(srcCaps);

  // Add elements to the pipeline
  gst_bin_add_many(GST_BIN(pipeline), source, rtpdepay, parse, decoder, convert, sink, nullptr);

  // Link elements
  if (!gst_element_link_many(source, rtpdepay, parse, decoder, convert, sink, nullptr)) {
    std::cerr << "Failed to link pipeline elements" << std::endl;
    gst_object_unref(pipeline);
    pipeline = nullptr;
    return false;
  }

  // Set up the message bus
  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, busCall, this);
  gst_object_unref(bus);

  // Start the pipeline
  GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    std::cerr << "Failed to start GStreamer pipeline" << std::endl;
    gst_object_unref(pipeline);
    pipeline = nullptr;
    return false;
  }

  videoEnabled = true;
  std::cout << "GStreamer pipeline created and started" << std::endl;
  return true;
}

bool VideoReceiverGst::frameGet(FrameData& frameData)
{
  std::lock_guard<std::mutex> lock(frameMutex);

  if (decodedFrames.empty()) {
    return false; // No frames available
  }

  // Get the newest frame (front of the deque)
  frameData = decodedFrames.front();
  return true;
}

void VideoReceiverGst::frameRelease(FrameData& frameData)
{
  std::lock_guard<std::mutex> lock(frameMutex);

  // Find the frame in our collection
  for (auto it = decodedFrames.begin(); it != decodedFrames.end(); ++it) {
    if (it->pixels == frameData.pixels) {
      // We found the frame, remove it and all older frames
      decodedFrames.erase(it, decodedFrames.end());
      break;
    }
  }

  // Reset the released frame data
  frameData = FrameData();
}

std::uint16_t VideoReceiverGst::getBufferFilled()
{
  return 0; // No jitter buffer in use
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/