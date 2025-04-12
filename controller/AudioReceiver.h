/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>
#include <cstdint>

#include <gst/gst.h>
#include <glib.h>

// Forward declarations
class Timer;

class AudioReceiver
{
 public:
  AudioReceiver();
  ~AudioReceiver();

  bool enableAudio(bool enable);

  // Method to consume audio data (replacing slot)
  void consumeAudio(std::vector<std::uint8_t>* audio);

 private:
  static gboolean busCall(GstBus* bus, GstMessage* msg, gpointer data);

  // GStreamer elements
  GstElement* pipeline;
  GstElement* source;

  // Playback state
  bool audioEnabled;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/