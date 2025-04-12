/*
 * Copyright 2020-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Controller-sdl.h"
#include "Timer.h"
#include "Message.h"
#include "Transmitter.h"
#include "VideoReceiver.h"
#include "AudioReceiver.h"

#include <iostream>
#include <string>
#include <cmath>

Controller_sdl::Controller_sdl(EventLoop& eventLoop, int &argc, char **argv)
  : Controller(eventLoop, argc, argv),
    window(nullptr),
    renderer(nullptr),
    videoTexture(nullptr),
    running(false),
    videoSources{"Camera", "Test"},
    currentVideoSource(0),
    enableLed(false),
    enableVideo(false),
    enableAudio(false),
    halfSpeed(true),
    cameraZoom(0),
    cameraFocus(0),
    videoQuality(0),
    videoBufferPercent(0),
    connectionStatus(0),
    rtt(0),
    resendTimeout(0),
    resentPackets(0),
    uptime(0),
    loadAvg(0.0f),
    wlanStrength(0),
    distance(0),
    temperature(0),
    current(0),
    voltage(0),
    calibrateSpeed(0),
    calibrateTurn(0),
    speed(0),
    turn(0),
    payloadRx(0),
    payloadTx(0),
    totalRx(0),
    totalTx(0),
    autoScroll(true)
{
  // Initialize ImGui debug text buffer
  debugTextBuffer.clear();
}

Controller_sdl::~Controller_sdl()
{
  cleanupImGui();
  cleanupSDL();
}

bool Controller_sdl::initSDL()
{
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
    return false;
  }

  // Initialize SDL_image
  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    std::cerr << "Error: SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
    return false;
  }

  // Create window
  window = SDL_CreateWindow(
    "Pleco Controller",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    1280, 720,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (!window) {
    std::cerr << "Error: Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
    return false;
  }

  // Create renderer
  renderer = SDL_CreateRenderer(
    window, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );
  if (!renderer) {
    std::cerr << "Error: Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
    return false;
  }

  // Set up rendering context
  SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);

  return true;
}

void Controller_sdl::cleanupSDL()
{
  if (videoTexture) {
    SDL_DestroyTexture(videoTexture);
    videoTexture = nullptr;
  }

  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
  }

  if (window) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }

  IMG_Quit();
  SDL_Quit();
}

bool Controller_sdl::initImGui()
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable docking

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  return true;
}

void Controller_sdl::cleanupImGui()
{
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void Controller_sdl::createGUI()
{
  std::cout << "Starting GUI creation..." << std::endl;

  // Call parent's createGUI method first
  Controller::createGUI();

  std::cout << "Initializing SDL..." << std::endl;
  // Initialize SDL and ImGui
  if (!initSDL()) {
    std::cerr << "Failed to initialize SDL" << std::endl;
    return;
  }
  std::cout << "SDL initialized successfully" << std::endl;

  std::cout << "Initializing ImGui..." << std::endl;
  if (!initImGui()) {
    std::cerr << "Failed to initialize ImGui" << std::endl;
    return;
  }
  std::cout << "ImGui initialized successfully" << std::endl;

  // Create video buffer percent update timer
  videoBufferTimer = std::make_shared<Timer>(eventLoop);
  videoBufferTimer->start(1000, [this]() {
    updateVideoBufferPercent();
  }, true);

  // Set up VideoReceiver callbacks
  if (vr) {
    std::cout << "Setting up video receiver callbacks..." << std::endl;
    vr->setPositionCallback([this](double x_percent, double y_percent) {
      updateCamera(x_percent, y_percent);
    });

    vr->setKeyEventCallback([this](SDL_KeyboardEvent* event) {
      handleKeyEvent(*event);
    });
    std::cout << "Video receiver callbacks set up" << std::endl;
  }

  // Set application as running
  running = true;
  std::cout << "GUI creation complete" << std::endl;
}

void Controller_sdl::connect(const std::string& host, std::uint16_t port)
{
  if (transmitter) {
    std::cout << "Transmitter already created, doing nothing." << std::endl;
    return;
  }

  transmitter = std::make_unique<Transmitter>(eventLoop, host, port);

  std::cout << "Setting transmitter callbacks" << std::endl;

  transmitter->setRttCallback([this](int ms) {
    updateRtt(ms);
  });

  transmitter->setResendTimeoutCallback([this](int ms) {
    updateResendTimeout(ms);
  });

  transmitter->setResentPacketsCallback([this](uint32_t resendCounter) {
    updateResentPackets(resendCounter);
  });

  transmitter->setNetworkRateCallback([this](int payloadRx, int totalRx, int payloadTx, int totalTx) {
    updateNetworkRate(payloadRx, totalRx, payloadTx, totalTx);
  });

  transmitter->setValueCallback([this](uint8_t type, uint16_t value) {
    updateValue(type, value);
  });

  transmitter->setPeriodicValueCallback([this](uint8_t type, uint16_t value) {
    updatePeriodicValue(type, value);
  });

  transmitter->setDebugCallback([this](std::string* debug) {
    showDebug(*debug);
  });

  transmitter->setConnectionStatusCallback([this](int status) {
    updateConnectionStatus(status);
  });

  transmitter->setVideoCallback([this](std::vector<uint8_t>* video, uint8_t index) {
    if (vr) vr->consumeVideo(video, index);
  });

  transmitter->setAudioCallback([this](std::vector<uint8_t>* audio) {
    if (ar) ar->consumeAudio(audio);
  });

  transmitter->initSocket();

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);
}

void Controller_sdl::run()
{
  // Main UI loop - no separate thread for ASIO
  running = true;

  std::cout << "Starting main loop..." << std::endl;

  // Set up UI timer to handle rendering
  auto uiTimer = std::make_shared<Timer>(eventLoop);
  uiTimer->start(16, [this]() {  // ~60 FPS
    if (!running) {
      std::cout << "Stopping main loop..." << std::endl;
      eventLoop.stop();  // Stop the event loop if we're no longer running
      return;
    }

    // Process SDL events in the main thread
    processEvents();

    // Render the UI
    renderFrame();

  }, true);  // Repeat the timer

  // Let the event loop handle both network and UI events
  // This will block until eventLoop.stop() is called
  eventLoop.run();
}

// New method to handle rendering
void Controller_sdl::renderFrame()
{
  // Start ImGui frame
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Render our GUI
  renderGUI();

  // Rendering
  ImGui::Render();
  SDL_RenderClear(renderer);

  // If we have video, render the video texture first
  if (vr && videoTexture) {
    SDL_RenderCopy(renderer, videoTexture, NULL, NULL);
  }

  // Render ImGui on top
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
  SDL_RenderPresent(renderer);
}

void Controller_sdl::processEvents()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // Pass events to ImGui
    ImGui_ImplSDL2_ProcessEvent(&event);

    // Handle application-specific events
    switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window)) {
          running = false;
        }
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        handleKeyEvent(event.key);
        break;

      case SDL_MOUSEMOTION:
        if (vr) {
          vr->handleMouseMove(&event.motion);
        }
        break;
    }
  }
}

void Controller_sdl::handleKeyEvent(SDL_KeyboardEvent& event)
{
  // Handle key events
  if (event.type == SDL_KEYDOWN) {
    switch (event.keysym.sym) {
      case SDLK_ESCAPE:
        running = false;
        break;

      case SDLK_l:
        toggleLed();
        break;

      case SDLK_v:
        toggleVideo();
        break;

      case SDLK_a:
        toggleAudio();
        break;

      case SDLK_h:
        toggleHalfSpeed();
        break;

      // Add more key handlers as needed
      default:
        updateMotor(event);
        break;
    }
  }
}

void Controller_sdl::updateMotor(SDL_KeyboardEvent& event)
{
  // Handle motor control with keyboard if needed
  // Simplified version - would likely need more complex handling in real app

  if (event.state == SDL_PRESSED) {
    switch (event.keysym.sym) {
      case SDLK_UP:
        sendSpeedTurn(50, turn);
        break;
      case SDLK_DOWN:
        sendSpeedTurn(-50, turn);
        break;
      case SDLK_LEFT:
        sendSpeedTurn(speed, -50);
        break;
      case SDLK_RIGHT:
        sendSpeedTurn(speed, 50);
        break;
    }
  } else if (event.state == SDL_RELEASED) {
    switch (event.keysym.sym) {
      case SDLK_UP:
      case SDLK_DOWN:
        sendSpeedTurn(0, turn);
        break;
      case SDLK_LEFT:
      case SDLK_RIGHT:
        sendSpeedTurn(speed, 0);
        break;
    }
  }
}

void Controller_sdl::renderGUI()
{
  // Set up dockspace for the main window
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

  // Render individual panels
  renderVideoPanel();
  renderControlPanel();
  renderStatusPanel();
  renderDebugPanel();

  ImGui::End();
}

void Controller_sdl::renderDebugPanel()
{
  ImGui::Begin("Debug");

  // Add auto-scroll checkbox
  ImGui::Checkbox("Auto-scroll", &autoScroll);

  // Debug text
  ImGui::BeginChild("DebugText", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextUnformatted(debugTextBuffer.begin(), debugTextBuffer.end());

  // Auto-scroll to bottom
  if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
  ImGui::End();
}

void Controller_sdl::renderStatusPanel()
{
  ImGui::Begin("Status");

  ImGui::Text("Connection: %s",
    connectionStatus == CONNECTION_STATUS_OK ? "Connected" :
    connectionStatus == CONNECTION_STATUS_RETRYING ? "Retrying" :
    connectionStatus == CONNECTION_STATUS_LOST ? "Lost" : "Unknown");

  ImGui::Text("RTT: %d ms", rtt);
  ImGui::Text("Resends: %u", resentPackets);
  ImGui::Text("Resend timeout: %d ms", resendTimeout);
  ImGui::Text("Uptime: %d sec", uptime);
  ImGui::Text("Load Avg: %.2f", loadAvg);
  ImGui::Text("WLAN Signal: %d%%", wlanStrength);
  ImGui::Text("Distance: %d m", distance);
  ImGui::Text("Temperature: %d°C", temperature);
  ImGui::Text("Current: %d A", current);
  ImGui::Text("Voltage: %d V", voltage);

  ImGui::Separator();

  ImGui::Text("Speed: %d%%", speed);
  ImGui::Text("Turn: %d%%", turn);

  ImGui::Separator();

  ImGui::Text("Network:");
  ImGui::Text("Payload Rx: %d", payloadRx);
  ImGui::Text("Total Rx: %d", totalRx);
  ImGui::Text("Payload Tx: %d", payloadTx);
  ImGui::Text("Total Tx: %d", totalTx);

  ImGui::Separator();

  ImGui::Text("Video Buffer: %d%%", videoBufferPercent);

  ImGui::End();
}

void Controller_sdl::renderControlPanel()
{
  ImGui::Begin("Controls");

  // Camera sliders
  int sliderX = 0;
  if (ImGui::SliderInt("Camera X", &sliderX, -90, 90)) {
    updateCameraX(sliderX);
  }

  int sliderY = 0;
  if (ImGui::SliderInt("Camera Y", &sliderY, -90, 90)) {
    updateCameraY(sliderY);
  }

  if (ImGui::SliderInt("Zoom", &cameraZoom, 0, 100)) {
    cameraZoomPercent = cameraZoom;
    sendCameraZoom();
  }

  if (ImGui::SliderInt("Focus", &cameraFocus, 0, 100)) {
    cameraFocusPercent = cameraFocus;
    sendCameraFocus();
  }

  if (ImGui::SliderInt("Video Quality", &videoQuality, 0, 4)) {
    sendVideoQuality();
  }

  ImGui::Separator();

  // Toggle buttons
  if (ImGui::Checkbox("LED", &enableLed)) {
    toggleLed();
  }

  if (ImGui::Checkbox("Video", &enableVideo)) {
    toggleVideo();
  }

  if (ImGui::Checkbox("Audio", &enableAudio)) {
    toggleAudio();
  }

  if (ImGui::Checkbox("Half Speed", &halfSpeed)) {
    toggleHalfSpeed();
  }

  ImGui::Separator();

  // Video source selector
  if (ImGui::BeginCombo("Video Source", videoSources[currentVideoSource].c_str())) {
    for (size_t i = 0; i < videoSources.size(); i++) {
      bool isSelected = (currentVideoSource == (int)i);
      if (ImGui::Selectable(videoSources[i].c_str(), isSelected)) {
        currentVideoSource = i;
        selectVideoSource(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::End();
}

void Controller_sdl::renderVideoPanel()
{
  ImGui::Begin("Video Feed");

  // Get content region for proper sizing
  ImVec2 contentSize = ImGui::GetContentRegionAvail();

  // If we have a video texture, display it
  if (videoTexture) {
    ImGui::Image((ImTextureID)(intptr_t)videoTexture, contentSize);
  } else {
    ImGui::TextWrapped("No video feed available. Enable video to start streaming.");
  }

  ImGui::End();
}

void Controller_sdl::updateRtt(int ms)
{
  rtt = ms;
  std::cout << "RTT: " << ms << "ms" << std::endl;
}

void Controller_sdl::updateResendTimeout(int ms)
{
  resendTimeout = ms;
  std::cout << "Resend timeout: " << ms << "ms" << std::endl;
}

void Controller_sdl::updateResentPackets(std::uint32_t resendCounter)
{
  resentPackets = resendCounter;
  std::cout << "Resent packets: " << resendCounter << std::endl;
}

void Controller_sdl::updateCalibrateSpeed(int percent)
{
  calibrateSpeed = percent;
  std::cout << "Calibrate speed: " << percent << "%" << std::endl;
}

void Controller_sdl::updateCalibrateTurn(int percent)
{
  calibrateTurn = percent;
  std::cout << "Calibrate turn: " << percent << "%" << std::endl;
}

void Controller_sdl::updateSpeed(int percent)
{
  speed = percent;
  std::cout << "Speed: " << percent << "%" << std::endl;
}

void Controller_sdl::updateTurn(int percent)
{
  turn = percent;
  std::cout << "Turn: " << percent << "%" << std::endl;
}

void Controller_sdl::toggleLed()
{
  enableLed = !enableLed;
  std::cout << "LED: " << (enableLed ? "ON" : "OFF") << std::endl;
  Controller::enableLed(enableLed);
}

void Controller_sdl::toggleVideo()
{
  enableVideo = !enableVideo;
  std::cout << "Video: " << (enableVideo ? "ON" : "OFF") << std::endl;
  Controller::enableVideo(enableVideo);

  // If video is enabled, create the video receiver
  if (enableVideo && vr) {
    vr->enableVideo(true);
  }
}

void Controller_sdl::toggleAudio()
{
  enableAudio = !enableAudio;
  std::cout << "Audio: " << (enableAudio ? "ON" : "OFF") << std::endl;
  Controller::enableAudio(enableAudio);

  // If audio is enabled, create the audio receiver
  if (enableAudio && ar) {
    ar->enableAudio(true);
  }
}

void Controller_sdl::toggleHalfSpeed()
{
  halfSpeed = !halfSpeed;
  std::cout << "Half speed: " << (halfSpeed ? "ON" : "OFF") << std::endl;
  motorHalfSpeed = halfSpeed;

  // Re-send current speed and turn values with new half-speed setting
  sendSpeedTurn(motorSpeed, motorTurn);
}

void Controller_sdl::selectVideoSource(int index)
{
  currentVideoSource = index;
  std::cout << "Selected video source: " << videoSources[index] << std::endl;
  sendVideoSource(index);
}

void Controller_sdl::updateNetworkRate(int pRx, int tRx, int pTx, int tTx)
{
  payloadRx = pRx;
  totalRx = tRx;
  payloadTx = pTx;
  totalTx = tTx;
}

void Controller_sdl::updateValue(std::uint8_t type, std::uint16_t value)
{
  switch(type) {
    case MessageSubtype::BatteryCurrent:
      current = value;
      break;
    case MessageSubtype::BatteryVoltage:
      voltage = value;
      break;
    case MessageSubtype::Distance:
      distance = value;
      break;
    case MessageSubtype::Temperature:
      temperature = value;
      break;
    case MessageSubtype::SignalStrength:
      wlanStrength = value;
      break;
    default:
      std::cout << "Unhandled value type: " << static_cast<int>(type) << " = " << value << std::endl;
      break;
  }
}

void Controller_sdl::updatePeriodicValue(std::uint8_t type, std::uint16_t value)
{
  switch(type) {
    case MessageSubtype::CPUUsage:
      loadAvg = value / 100.0f;
      break;
    case MessageSubtype::Uptime:
      uptime = value;
      break;
    default:
      std::cout << "Unhandled periodic value type: " << static_cast<int>(type) << " = " << value << std::endl;
      break;
  }
}

void Controller_sdl::showDebug(const std::string& msg)
{
  // Add to debug messages list
  debugMessages.push_back(msg);

  // Limit the number of messages
  if (debugMessages.size() > 100) {
    debugMessages.erase(debugMessages.begin());
  }

  // Add to ImGui text buffer
  debugTextBuffer.append(msg.c_str());
  debugTextBuffer.append("\n");

  // Print to console as well
  std::cout << "Debug: " << msg << std::endl;
}

void Controller_sdl::updateConnectionStatus(int status)
{
  connectionStatus = status;
  std::cout << "Connection status: " << status << std::endl;
}

void Controller_sdl::updateCamera(double x_percent, double y_percent)
{
  // Convert percentages to angles (-90 to 90 degrees)
  double newX = (x_percent * 2.0 - 1.0) * 90.0;
  double newY = (y_percent * 2.0 - 1.0) * 90.0;

  // Update internal state
  cameraX = newX;
  cameraY = newY;

  // Send camera position update to device
  sendCameraXY();
}

void Controller_sdl::updateCameraX(int degree)
{
  cameraX = degree;
  sendCameraXY();
}

void Controller_sdl::updateCameraY(int degree)
{
  cameraY = degree;
  sendCameraXY();
}

void Controller_sdl::updateVideoBufferPercent()
{
  if (vr) {
    videoBufferPercent = vr->getBufferFilled();
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
