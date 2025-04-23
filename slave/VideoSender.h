/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Hardware.h"
#include "Event.h"

#include <vector>
#include <cstdint>
#include <functional>
#include <string>
#include <memory>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

class VideoSender
{
 public:
  VideoSender(EventLoop& eventLoop, Hardware* hardware);
  ~VideoSender();

  bool enableSending(bool enable);
  void setVideoSource(int index);
  void setVideoQuality(std::uint16_t quality);

  // Callback type for video data
  using VideoCallback = std::function<void(std::vector<std::uint8_t>* video)>;

  // Set callback for video data
  void setVideoCallback(VideoCallback callback);

 private:
  void setBitrate(int bitrate);
  void emitVideo(std::vector<std::uint8_t>* data);
  void launchObjectDetection();
  void processObjectDetectionOutput();
  void handleObjectDetectionExit(int exitCode);

  static GstFlowReturn newBufferCB(GstAppSink* sink, gpointer user_data);
  static GstFlowReturn newBufferOBCB(GstAppSink* sink, gpointer user_data);

  // The EventLoop for async operations
  EventLoop& eventLoop;

  // GStreamer elements
  GstElement* pipeline;
  GstElement* encoder;

  // Process related members
  std::unique_ptr<asio::posix::stream_descriptor> processStdout;
  std::unique_ptr<asio::posix::stream_descriptor> processStderr;
  std::unique_ptr<asio::posix::stream_descriptor> processStdin;
  int processPid;
  bool processReady;

  // Buffer for process data
  std::vector<std::uint8_t> processBuffer;
  std::uint8_t ODdata[6];

  // Video properties
  std::string videoSource;
  int bitrate;
  std::uint16_t quality;
  std::uint8_t index;

  // Hardware reference
  Hardware* hardware;

  // Callback for video data
  VideoCallback videoCallback;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
