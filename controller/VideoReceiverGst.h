/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "IVideoReceiver.h"
#include <vector>
#include <deque>
#include <mutex>
#include <cstdint>
#include <gst/gst.h>
#include <glib.h>

// Forward declarations
class Timer;

class VideoReceiverGst : public IVideoReceiver
{
 public:
  VideoReceiverGst();
  ~VideoReceiverGst() override;

  // Implement IVideoReceiver interface
  bool init() override;
  void deinit() override;
  void consumeBitStream(std::vector<std::uint8_t>* video) override;
  std::uint16_t getBufferFilled() override;

  // Get the latest decoded frame data
  // Returns true if a valid frame is available, false otherwise
  // The implementation must ensure the frame data remains valid until frameRelease is called
  bool frameGet(FrameData& frameData) override;

  // Free resources associated with frameData and all older frames
  void frameRelease(FrameData& frameData) override;

 private:
  bool createPipeline();

  static gboolean busCall(GstBus* bus, GstMessage* msg, gpointer data);

  // Callback for new sample from the GStreamer pipeline
  static GstFlowReturn onNewSample(GstElement* sink, gpointer userData);

  // Frame management
  void storeDecodedFrame(const void* data, int width, int height, int stride, int64_t timestamp);
  void cleanupOldFrames();

  // GStreamer elements
  GstElement* pipeline;
  GstElement* source;
  GstElement* sink;

  // Thread safety
  std::mutex frameMutex;

  // Storage for decoded frames (newest first)
  std::deque<FrameData> decodedFrames;


  // Video state
  bool videoEnabled;
  bool initialized;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/