/*
 * Copyright 2020-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Controller.h"
#include "Event.h"

#include <string>
#include <memory>
#include <cstdint>
#include <vector>

// SDL includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// ImGui includes
#include "imgui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

// Forward declarations
class Timer;

class Controller_sdl : public Controller
{
 public:
  Controller_sdl(EventLoop& eventLoop, int &argc, char **argv);
  ~Controller_sdl();

  void createGUI();
  void connect(const std::string& host, std::uint16_t port);

  // Run the UI loop
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

  // Callback methods (replacing slots)
  void updateRtt(int ms);
  void updateResendTimeout(int ms);
  void updateResentPackets(std::uint32_t resendCounter);
  void updateCalibrateSpeed(int percent);
  void updateCalibrateTurn(int percent);
  void updateSpeed(int percent);
  void updateTurn(int percent);
  void toggleLed();
  void toggleVideo();
  void toggleAudio();
  void toggleHalfSpeed();
  void selectVideoSource(int index);
  void updateNetworkRate(int payloadRx, int totalRx, int payloadTx, int totalTx);
  void updateValue(std::uint8_t type, std::uint16_t value);
  void updatePeriodicValue(std::uint8_t type, std::uint16_t value);
  void showDebug(const std::string& msg);
  void updateConnectionStatus(int status);
  void updateCamera(double x_percent, double y_percent);
  void updateCameraX(int degree);
  void updateCameraY(int degree);

  // Timer callbacks
  void updateVideoBufferPercent();

  std::shared_ptr<Timer> videoBufferTimer;

  // SDL and ImGui resources
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* videoTexture;

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

  // Status information
  int rtt;
  int resendTimeout;
  std::uint32_t resentPackets;
  int uptime;
  float loadAvg;
  int wlanStrength;
  int distance;
  int temperature;
  int current;
  int voltage;

  // Motor control
  int calibrateSpeed;
  int calibrateTurn;
  int speed;
  int turn;

  // Network stats
  int payloadRx;
  int payloadTx;
  int totalRx;
  int totalTx;

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