/*
 * Copyright 2012-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Message.h"
#include "Event.h"
#include "Timer.h"

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <map>
#include <memory>
#include <atomic>
#include <asio.hpp>

#define CONNECTION_STATUS_OK          0x1
#define CONNECTION_STATUS_RETRYING    0x2
#define CONNECTION_STATUS_LOST        0x3

class Transmitter
{
 public:
  // Callback types
  using RttCallback = std::function<void(int ms)>;
  using ResendTimeoutCallback = std::function<void(int ms)>;
  using ResentPacketsCallback = std::function<void(uint32_t resendCounter)>;
  using VideoCallback = std::function<void(std::vector<uint8_t>* video)>;
  using AudioCallback = std::function<void(std::vector<uint8_t>* audio)>;
  using DebugCallback = std::function<void(std::string* debug)>;
  using ValueCallback = std::function<void(uint8_t type, uint16_t value)>;
  using PeriodicValueCallback = std::function<void(uint8_t type, uint16_t value)>;
  using NetworkRateCallback = std::function<void(int payloadRx, int totalRx, int payloadTx, int totalTx)>;
  using ConnectionStatusCallback = std::function<void(int status)>;

  // Message handler function prototype
  using messageHandler = void (Transmitter::*)(Message &msg);

  // Modified constructor to take EventLoop reference instead of creating its own
  Transmitter(EventLoop& eventLoop, const std::string& host, uint16_t port);
  ~Transmitter();

  void initSocket();
  void enableAutoPing(bool enable);

  // Methods to set callbacks
  void setRttCallback(RttCallback callback);
  void setResendTimeoutCallback(ResendTimeoutCallback callback);
  void setResentPacketsCallback(ResentPacketsCallback callback);
  void setVideoCallback(VideoCallback callback);
  void setAudioCallback(AudioCallback callback);
  void setDebugCallback(DebugCallback callback);
  void setValueCallback(ValueCallback callback);
  void setPeriodicValueCallback(PeriodicValueCallback callback);
  void setNetworkRateCallback(NetworkRateCallback callback);
  void setConnectionStatusCallback(ConnectionStatusCallback callback);

  // Public methods
  void sendPing();
  void sendVideo(std::vector<uint8_t>* video);
  void sendAudio(std::vector<uint8_t>* audio);
  void sendDebug(std::string* debug);
  void sendValue(uint8_t type, uint16_t value);
  void sendPeriodicValue(uint8_t type, uint16_t value);

 private:
  void readPendingDatagrams();
  void printError(int error);
  void sendMessage(Message* msg);
  void resendMessage(Message* msg);
  void updateRate();
  void connectionTimeout();

  void printData(std::vector<uint8_t>* data);
  void parseData(std::vector<uint8_t>* data);
  void handleACK(Message& msg);
  void handlePing(Message& msg);
  void handleVideo(Message& msg);
  void handleAudio(Message& msg);
  void handleDebug(Message& msg);
  void handleValue(Message& msg);
  void handlePeriodicValue(Message& msg);
  void sendACK(Message& incoming);
  void startResendTimer(Message* msg);
  void startRTTimer(Message* msg);
  void startConnectionTimeout();

  // Reference to shared EventLoop instead of owning an io_context
  EventLoop& eventLoop;

  // ASIO networking components (no io_context - we use the one from EventLoop)
  asio::ip::udp::socket socket;
  asio::ip::udp::endpoint remote_endpoint;

  // Add this to the private section:
  std::vector<std::uint8_t> receiveBuffer;

  std::string relayHost;
  uint16_t relayPort;
  int resendTimeoutMs;
  uint32_t resendCounter;

  // Map for managing resend timers and callbacks
  std::map<uint16_t, std::shared_ptr<Timer>> resendTimers;
  std::map<uint16_t, std::shared_ptr<std::chrono::steady_clock::time_point>> rtTimers;
  std::map<uint16_t, std::unique_ptr<Message>> resendMessages;
  messageHandler messageHandlers[MSG_TYPE_MAX] = {nullptr};

  std::shared_ptr<Timer> connectionTimeoutTimer;
  int connectionStatus;

  std::shared_ptr<Timer> autoPing;

  // TX/RX rate
  int payloadSent;
  int payloadRecv;
  int totalSent;
  int totalRecv;
  std::shared_ptr<Timer> rateTimer;
  std::chrono::steady_clock::time_point rateTime;

  // No need for io_thread since the EventLoop handles this
  std::atomic<bool> running;

  // Callback
  RttCallback onRtt;
  ResendTimeoutCallback onResendTimeout;
  ResentPacketsCallback onResentPackets;
  VideoCallback onVideo;
  AudioCallback onAudio;
  DebugCallback onDebug;
  ValueCallback onValue;
  PeriodicValueCallback onPeriodicValue;
  NetworkRateCallback onNetworkRate;
  ConnectionStatusCallback onConnectionStatus;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
