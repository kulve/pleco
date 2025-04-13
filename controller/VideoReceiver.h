/*
 * Copyright 2017-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <vector>
#include <cstdint>

#include <SDL2/SDL.h>
#include <gst/gst.h>
#include <glib.h>

// Forward declarations
class Timer;

class VideoReceiver
{
 public:
  VideoReceiver();
  ~VideoReceiver();

  bool enableVideo(bool enable);
  std::uint16_t getBufferFilled();

  // Create SDL texture from the video frame for rendering
  SDL_Texture* createTextureFromFrame(SDL_Renderer* renderer);

  // Callbacks
  using PositionCallback = std::function<void(double x_percent, double y_percent)>;
  using KeyEventCallback = std::function<void(SDL_KeyboardEvent* event)>;
  using FrameCallback = std::function<void(const void* data, int width, int height)>;

  // Set callbacks
  void setPositionCallback(PositionCallback callback);
  void setKeyEventCallback(KeyEventCallback callback);
  void setFrameCallback(FrameCallback callback) { frameCallback = callback; }

  // Method to consume video data
  void consumeVideo(std::vector<std::uint8_t>* video, std::uint8_t index);

  // Methods to handle SDL events
  void handleMouseMove(SDL_MouseMotionEvent* event);
  void handleKeyPress(SDL_KeyboardEvent* event);
  void handleKeyRelease(SDL_KeyboardEvent* event);

  FrameCallback frameCallback;

 private:
  static gboolean busCall(GstBus* bus, GstMessage* msg, gpointer data);

  // GStreamer elements
  GstElement* pipeline;
  GstElement* source;
  GstElement* sink;

  // Callbacks
  PositionCallback positionCallback;
  KeyEventCallback keyEventCallback;

  // Video dimensions
  int width;
  int height;

  // Frame data
  std::vector<std::uint8_t> frameData;

  // Playback state
  bool videoEnabled;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/