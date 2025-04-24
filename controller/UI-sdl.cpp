/*
 * Copyright 2020-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "UI-sdl.h"

#include <iostream>
#include <string>
#include <cmath>

#include "Controller.h"
#include "IVideoReceiver.h"
#include "AudioReceiver.h"

UI_Sdl::UI_Sdl(Controller& controller, int &/*argc*/, char ** /*argv*/):
    ctrl(controller),
    renderer(nullptr),
    videoTexture(nullptr),
    frameDataPrev(nullptr),
    frameDataCurrent(nullptr),
    running(false),
    videoSources{"Camera", "Test"},
    currentVideoSource(0),
    enableVideo(false),
    enableAudio(false),
    halfSpeed(true),
    cameraZoom(0),
    cameraFocus(0),
    videoQuality(0),
    videoBufferPercent(0),
    autoScroll(true)
{
  // Initialize ImGui debug text buffer
  debugTextBuffer.clear();
}

UI_Sdl::~UI_Sdl()
{
  if (frameDataPrev != nullptr) {
    ctrl.videoFrameRelease(*frameDataPrev);
    delete frameDataPrev;
  }

  if (frameDataCurrent != nullptr) {
    ctrl.videoFrameRelease(*frameDataCurrent);
    delete frameDataCurrent;
  }

  cleanupImGui();
  cleanupSDL();
}

bool UI_Sdl::initSDL()
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

void UI_Sdl::cleanupSDL()
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

bool UI_Sdl::initImGui()
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

void UI_Sdl::cleanupImGui()
{
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void UI_Sdl::createGUI()
{

  std::cout << "Initializing SDL..." << std::endl;
  // Initialize SDL and ImGui
  if (!initSDL()) {
    std::cerr << "Failed to initialize SDL" << std::endl;
    return;
  }

  std::cout << "Initializing ImGui..." << std::endl;
  if (!initImGui()) {
    std::cerr << "Failed to initialize ImGui" << std::endl;
    return;
  }

  // Set application as running
  running = true;
  std::cout << "GUI creation complete" << std::endl;
}


void UI_Sdl::run()
{
  // Main UI loop
  running = true;

  std::cout << "Starting main SDL loop..." << std::endl;

  // Traditional SDL main loop
  while (running) {
    // Process SDL events
    processEvents();

    // Render the frame
    renderFrame();

    // FIXME: remove this: Control frame rate (if not using SDL_RENDERER_PRESENTVSYNC)
    SDL_Delay(16); // ~60 FPS
  }

  std::cout << "Main SDL loop finished, stopping event loop..." << std::endl;
}

void UI_Sdl::renderFrame()
{
    // Start by clearing the renderer with a distinctive color
    SDL_SetRenderDrawColor(renderer, 50, 0, 50, 255); // Purple background
    SDL_RenderClear(renderer);

    // IMPORTANT: Get the latest frame FIRST, BEFORE trying to render
    if (enableVideo) {
        // Get the latest frame from the controller
        IVideoReceiver::FrameData frameDataNew;
        if (ctrl.videoFrameGet(frameDataNew)) {
            // Update the video texture with the new frame data
            updateVideoTexture(frameDataNew.pixels, frameDataNew.width, frameDataNew.height);

            // Free the previous frame if it exists
            if (frameDataPrev != nullptr) {
                ctrl.videoFrameRelease(*frameDataPrev);
                delete frameDataPrev;
            }

            // Move the current frame to previous, and store the new frame
            frameDataPrev = frameDataCurrent;
            frameDataCurrent = new IVideoReceiver::FrameData(frameDataNew);
        }

        // AFTER getting frames, now try to render
        // Get window size
        int w, h;
        SDL_GetWindowSize(window, &w, &h);

        // Only try to render if we have valid video dimensions
        if (videoWidth > 0 && videoHeight > 0 && videoTexture != nullptr) {
            // Calculate aspect-preserving dimensions to fit in window
            float aspectRatio = static_cast<float>(videoWidth) / videoHeight;
            int destW = w;
            int destH = static_cast<int>(w / aspectRatio);

            // If height exceeds window height, scale down
            if (destH > h) {
                destH = h;
                destW = static_cast<int>(h * aspectRatio);
            }

            // Center the video on screen
            int x = (w - destW) / 2;
            int y = (h - destH) / 2;
            SDL_Rect dest = {x, y, destW, destH};

            // Draw a bright red border around the video area
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &dest);

            // Draw the video with proper scaling
            SDL_RenderCopy(renderer, videoTexture, nullptr, &dest);

            // Reduce logging frequency - uncomment if needed for debugging
            static int frameCounter = 0;
            if (frameCounter++ % 60 == 0) {  // Log only every 60 frames
                 std::cout << "Video rendered: " << videoWidth << "x" << videoHeight
                     << " -> " << destW << "x" << destH
                     << " at position: " << x << "," << y << std::endl;
            }
        } else {
            // No video texture available, show a message
            std::cout << "No video texture available, size: "
                      << videoWidth << "x" << videoHeight
                      << ", texture: " << videoTexture << std::endl;
        }
    }

    // Rest of the ImGui rendering...
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render our ImGui windows
    renderGUI();

    // Finish ImGui rendering
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    // Present the final composited frame
    SDL_RenderPresent(renderer);
}

void UI_Sdl::processEvents()
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
    }
  }
}

void UI_Sdl::handleKeyEvent(SDL_KeyboardEvent& event)
{
  // Handle key events
  if (event.type == SDL_KEYDOWN) {
    switch (event.keysym.sym) {
      case SDLK_ESCAPE:
        running = false;
        break;

      case SDLK_l:
        ctrl.setLed(enableLed);
        break;

      case SDLK_v:
        ctrl.setVideo(enableVideo);
        break;

      case SDLK_a:
        ctrl.setAudio(enableAudio);
        break;

      case SDLK_h:
        ctrl.setHalfSpeed(halfSpeed);
        break;

      // Add more key handlers as needed
      default:
        updateMotor(event);
        break;
    }
  }
}

void UI_Sdl::updateMotor(SDL_KeyboardEvent& event)
{
  // Handle motor control with keyboard if needed
  // Simplified version - would likely need more complex handling in real app

  if (event.state == SDL_PRESSED) {
    switch (event.keysym.sym) {
      case SDLK_UP:
        ctrl.setSpeedTurn(50, turn);
        break;
      case SDLK_DOWN:
        ctrl.setSpeedTurn(-50, turn);
        break;
      case SDLK_LEFT:
        ctrl.setSpeedTurn(speed, -50);
        break;
      case SDLK_RIGHT:
        ctrl.setSpeedTurn(speed, 50);
        break;
    }
  } else if (event.state == SDL_RELEASED) {
    switch (event.keysym.sym) {
      case SDLK_UP:
      case SDLK_DOWN:
        ctrl.setSpeedTurn(0, turn);
        break;
      case SDLK_LEFT:
      case SDLK_RIGHT:
        ctrl.setSpeedTurn(speed, 0);
        break;
    }
  }
}

void UI_Sdl::renderGUI()
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

void UI_Sdl::renderDebugPanel()
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

void UI_Sdl::renderStatusPanel()
{
  ImGui::Begin("Status");

  int32_t stats[Stats::Container::size()];
  ctrl.getStats(stats);

  if (stats[CTRL_STATS_CONNECTION_STATUS] != CONNECTION_STATUS_OK) {
    if (stats[CTRL_STATS_CONNECTION_STATUS] != ctrlStats[CTRL_STATS_CONNECTION_STATUS]) {
      std::cout << "Connection lost, stopping video and audio" << std::endl;
      enableVideo = false;
      ctrl.setVideo(enableVideo);
      enableAudio = false;
      ctrl.setAudio(enableAudio);
    }
  }

  // Update the stored stats
  std::copy(stats, stats + Stats::Container::size(), ctrlStats);

  ImGui::Text("Connection: %s",
    stats[CTRL_STATS_CONNECTION_STATUS] == CONNECTION_STATUS_OK ? "Connected" :
    stats[CTRL_STATS_CONNECTION_STATUS] == CONNECTION_STATUS_RETRYING ? "Retrying" :
    stats[CTRL_STATS_CONNECTION_STATUS] == CONNECTION_STATUS_LOST ? "Lost" : "Unknown");

  ImGui::Text("RTT: %d ms", stats[CTRL_STATS_RTT]);
  ImGui::Text("Resends: %u", stats[CTRL_STATS_RESENT_PACKETS]);
  ImGui::Text("Resend timeout: %d ms", stats[CTRL_STATS_RESEND_TIMEOUT]);
  ImGui::Text("Uptime: %d sec", stats[CTRL_STATS_UPTIME]);
  ImGui::Text("Load Avg: %d", stats[CTRL_STATS_LOAD_AVG]);
  ImGui::Text("WLAN Signal: %d%%", stats[CTRL_STATS_WLAN_STRENGTH]);
  ImGui::Text("Distance: %d m", stats[CTRL_STATS_DISTANCE]);
  ImGui::Text("Temperature: %d°C", stats[CTRL_STATS_TEMPERATURE]);
  ImGui::Text("Current: %d A", stats[CTRL_STATS_CURRENT]);
  ImGui::Text("Voltage: %d V", stats[CTRL_STATS_VOLTAGE]);


  ImGui::Separator();

  ImGui::Text("Speed: %d%%", speed);
  ImGui::Text("Turn: %d%%", turn);

  ImGui::Separator();

  ImGui::Text("Network:");
  ImGui::Text("Payload Rx: %d", stats[CTRL_STATS_PAYLOAD_RX]);
  ImGui::Text("Total Rx: %d", stats[CTRL_STATS_TOTAL_RX]);
  ImGui::Text("Payload Tx: %d", stats[CTRL_STATS_PAYLOAD_TX]);
  ImGui::Text("Total Tx: %d", stats[CTRL_STATS_TOTAL_TX]);

  ImGui::Separator();

  //updateVideoBufferPercent();
  ImGui::Text("Video Buffer: %d%%", videoBufferPercent);

  ImGui::End();
}

void UI_Sdl::renderControlPanel()
{
  ImGui::Begin("Controls");

  // Camera sliders
  if (ImGui::SliderInt("Camera X", &sliderX, -90, 90)) {
    ctrl.setCameraX(sliderX);
  }

  if (ImGui::SliderInt("Camera Y", &sliderY, -90, 90)) {
    ctrl.setCameraY(sliderY);
  }

  if (ImGui::SliderInt("Zoom", &cameraZoom, 0, 100)) {
    ctrl.setCameraZoom(cameraZoom);
  }

  if (ImGui::SliderInt("Focus", &cameraFocus, 0, 100)) {
    ctrl.setCameraFocus(cameraFocus);
  }

  if (ImGui::SliderInt("Video Quality", &videoQuality, 0, 4)) {
    ctrl.setVideoQuality(videoQuality);
  }

  ImGui::Separator();

  // Toggle buttons
  if (ImGui::Checkbox("LED", &enableLed)) {
    ctrl.setLed(enableLed);
  }

  if (ImGui::Checkbox("Video", &enableVideo)) {
    ctrl.setVideo(enableVideo);
  }

  if (ImGui::Checkbox("Audio", &enableAudio)) {
    ctrl.setAudio(enableAudio);
  }

  if (ImGui::Checkbox("Half Speed", &halfSpeed)) {
    ctrl.setHalfSpeed(halfSpeed);
  }

  ImGui::Separator();

  // Video source selector
  if (ImGui::BeginCombo("Video Source", videoSources[currentVideoSource].c_str())) {
    for (size_t i = 0; i < videoSources.size(); i++) {
      bool isSelected = (currentVideoSource == (int)i);
      if (ImGui::Selectable(videoSources[i].c_str(), isSelected)) {
        currentVideoSource = i;
        ctrl.setVideoSource(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::End();
}

void UI_Sdl::updateVideoTexture(const void* data, int width, int height)
{
  std::cout << "Updating video texture with dimensions: " << width << "x" << height << std::endl;
  if (data == nullptr || width <= 0 || height <= 0) {
    std::cerr << "Warning: Invalid texture data" << std::endl;
    return;
  }

  // If texture doesn't exist or dimensions have changed, recreate it
  if (!videoTexture || this->videoWidth != width || this->videoHeight != height) {
    std::cout << "Creating new texture of size " << width << "x" << height << std::endl;
    if (videoTexture) {
      SDL_DestroyTexture(videoTexture);
    }

    // Use RGBA format to match GStreamer's output
    Uint32 pixelFormat = SDL_PIXELFORMAT_RGBA32;

    videoTexture = SDL_CreateTexture(
      renderer,
      pixelFormat,
      SDL_TEXTUREACCESS_STREAMING,
      width, height
    );

    if (!videoTexture) {
      std::cerr << "Failed to create video texture: " << SDL_GetError() << std::endl;
      return;
    }

    // Explicitly set texture properties
    SDL_SetTextureBlendMode(videoTexture, SDL_BLENDMODE_BLEND);

    // Make sure texture alpha is fully opaque
    SDL_SetTextureAlphaMod(videoTexture, 255);

    // Query the actual format that was created
    Uint32 format;
    int access, w, h;
    if (SDL_QueryTexture(videoTexture, &format, &access, &w, &h) == 0) {
      std::cout << "Texture created successfully with dimensions: " << w << "x" << h << std::endl;
    }

    this->videoWidth = width;
    this->videoHeight = height;
  }

  // Debug: Check first few bytes of pixel data
  const uint8_t* pixels = static_cast<const uint8_t*>(data);
  int stride = width * 4; // 4 bytes per pixel for RGBA

  std::cout << "First pixel RGBA: ("
            << static_cast<int>(pixels[0]) << ", "
            << static_cast<int>(pixels[1]) << ", "
            << static_cast<int>(pixels[2]) << ", "
            << static_cast<int>(pixels[3]) << ")" << std::endl;

  // Update the texture with new data - use correct stride
  SDL_UpdateTexture(videoTexture, nullptr, data, stride);
  std::cout << "Texture updated successfully" << std::endl;
}

void UI_Sdl::renderVideoPanel()
{
  // Use a solid (not transparent) background for the video window
  ImGui::Begin("Video Feed");

  // Get content region for proper sizing
  ImVec2 contentSize = ImGui::GetContentRegionAvail();

  // If we have a video texture, display it
  if (videoTexture) {
    // Calculate scaled dimensions that preserve aspect ratio
    float aspectRatio = static_cast<float>(videoWidth) / videoHeight;
    float panelAspectRatio = contentSize.x / contentSize.y;

    ImVec2 displaySize;
    if (aspectRatio > panelAspectRatio) {
      // Video is wider than panel
      displaySize.x = contentSize.x;
      displaySize.y = contentSize.x / aspectRatio;
    } else {
      // Video is taller than panel
      displaySize.y = contentSize.y;
      displaySize.x = contentSize.y * aspectRatio;
    }

    // Calculate center position
    ImVec2 pos(
      ImGui::GetCursorPosX() + (contentSize.x - displaySize.x) * 0.5f,
      ImGui::GetCursorPosY() + (contentSize.y - displaySize.y) * 0.5f
    );

    // Set cursor position for centered image
    ImGui::SetCursorPos(pos);

    // Display the image with proper sizing
    ImGui::Image((ImTextureID)(intptr_t)videoTexture, displaySize);

    std::cout << "ImGui video panel: Texture " << videoWidth << "x" << videoHeight
              << " rendered at " << displaySize.x << "x" << displaySize.y
              << " position: " << pos.x << "," << pos.y
              << " (panel size: " << contentSize.x << "x" << contentSize.y << ")" << std::endl;
  } else {
    ImGui::TextWrapped("No video feed available. Enable video to start streaming.");
  }

  ImGui::End();
}

void UI_Sdl::showDebug(const std::string& msg)
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

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
