/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Hardware.h"

#include <vector>
#include <cstdint>
#include <functional>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>

class AudioSender
{
 public:
  AudioSender(Hardware *hardware);
  ~AudioSender();

  bool enableSending(bool enable);

  // Callback type for audio data
  using AudioCallback = std::function<void(std::vector<std::uint8_t>*)>;

  // Set callback for audio data
  void setAudioCallback(AudioCallback callback);

 private:
  void emitAudio(std::vector<std::uint8_t> *data);
  static GstFlowReturn newBufferCB(GstAppSink *sink, gpointer user_data);

  GstElement *pipeline;
  Hardware *hardware;
  GstElement *encoder;

  // Callback for audio data
  AudioCallback audioCallback;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
