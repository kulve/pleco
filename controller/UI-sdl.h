/*
 * Copyright 2020-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

// SDL includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// ImGui includes
#include "imgui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include "Controller.h"

// Forward declarations
class Timer;

class UI_Sdl
{
 public:
  UI_Sdl(Controller& controller, int &argc, char **argv);
  ~UI_Sdl();

  void createGUI();
  void run();

 private:
  // SDL and ImGui setup/teardown
  bool initSDL();
  void cleanupSDL();
  bool initImGui();
  void cleanupImGui();

  // Event handling
  void processEvents();
  void renderFrame();
  void handleKeyEvent(SDL_KeyboardEvent& event);
  void updateMotor(SDL_KeyboardEvent& event);

  // ImGui UI rendering
  void renderGUI();
  void renderDebugPanel();
  void renderStatusPanel();
  void renderControlPanel();
  void renderVideoPanel();
  void updateVideoTexture(const void* data, int width, int height);

  // Callback methods
  void showDebug(const std::string& msg);

  // Timer callbacks
  void updateVideoBufferPercent();

  Controller& ctrl;

  std::shared_ptr<Timer> videoBufferTimer;

  // SDL and ImGui resources
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* videoTexture;
  int videoWidth = 0;
  int videoHeight = 0;

  // Application state
  bool running;
  std::vector<std::string> videoSources;
  int currentVideoSource;
  bool enableLed;
  bool enableVideo;
  bool enableAudio;
  bool halfSpeed;
  int cameraZoom;
  int cameraFocus;
  int videoQuality;
  int videoBufferPercent;
  int connectionStatus;
  int speed;
  int turn;
  int sliderX = 0;
  int sliderY = 0;

  // Debug messages
  std::vector<std::string> debugMessages;
  ImGuiTextBuffer debugTextBuffer;
  bool autoScroll;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/