/*
 * Copyright 2015-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Transmitter.h"
#include "Message.h"

#include <iostream>
#include <iomanip>

#define RESEND_TIMEOUT_DEFAULT 1000



Transmitter::Transmitter(EventLoop& eventLoop, const std::string& host, uint16_t port):
  eventLoop(eventLoop),
  socket(eventLoop.context()),
  relayHost(host),
  relayPort(port),
  resendTimeoutMs(RESEND_TIMEOUT_DEFAULT),
  resendCounter(0),
  connectionStatus(CONNECTION_STATUS_LOST),
  payloadSent(0),
  payloadRecv(0),
  totalSent(0),
  totalRecv(0),
  running(true)
{
  std::cout << "Transmitter initializing with host: " << host << ", port: " << port << std::endl;


  // Set message handlers
  messageHandlers[MessageType::Ack]            = &Transmitter::handleACK;
  messageHandlers[MessageType::Ping]           = &Transmitter::handlePing;
  messageHandlers[MessageType::Video]          = &Transmitter::handleVideo;
  messageHandlers[MessageType::Audio]          = &Transmitter::handleAudio;
  messageHandlers[MessageType::Debug]          = &Transmitter::handleDebug;
  messageHandlers[MessageType::Value]          = &Transmitter::handleValue;
  messageHandlers[MessageType::PeriodicValue]  = &Transmitter::handlePeriodicValue;
}

Transmitter::~Transmitter()
{
  std::cout << "Transmitter destructor" << std::endl;

  // Stop receiving data
  running = false;

  // Clean up timer objects (which involve async operations)
  if (connectionTimeoutTimer) connectionTimeoutTimer->stop();
  if (autoPing) autoPing->stop();
  if (rateTimer) rateTimer->stop();

  // Stop any resend timers
  for (auto& [key, timer] : resendTimers) {
    if (timer) timer->stop();
  }

  // Clear all maps in one go
  rtTimers.clear();
  resendTimers.clear();
  resendMessages.clear();

  // Reset shared_ptr members
  connectionTimeoutTimer.reset();
  autoPing.reset();
  rateTimer.reset();

  // Clean up the message handlers
  for (std::size_t i = 0; i < MSG_TYPE_MAX; i++) {
    messageHandlers[i] = nullptr;
  }
}

void Transmitter::initSocket()
{
  std::cout << "Initializing socket" << std::endl;

  asio::error_code ec;
  // NOLINTNEXTLINE(bugprone-unused-return-value)
  socket.open(asio::ip::udp::v4(), ec);
  if (ec) {
    std::cerr << "Failed to open socket: " << ec.message() << std::endl;
    return;
  }

  // NOLINTNEXTLINE(bugprone-unused-return-value)
  socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), 0), ec);
  if (ec) {
    std::cerr << "Failed to bind socket: " << ec.message() << std::endl;
    return;
  }

  // Get the local endpoint
  asio::ip::udp::endpoint local_endpoint = socket.local_endpoint(ec);
  if (!ec) {
    std::cout << "Local address: " << local_endpoint.address().to_string() << std::endl;
    std::cout << "Local port: " << local_endpoint.port() << std::endl;
  }

  // Start async read operation
  readPendingDatagrams();

  // Start RX/TX rate timer
  rateTimer = std::make_shared<Timer>(eventLoop);
  rateTime = std::chrono::steady_clock::now();
  rateTimer->start(1000, [this]() { updateRate(); }, true);
}

void Transmitter::enableAutoPing(bool enable)
{
  if (!enable) {
    // Disable autopinging, if running
    if (autoPing) {
      autoPing->stop();
      autoPing.reset();
    }
    return;
  }

  // Start autopinging, if not already running
  if (autoPing) {
    return;
  }

  autoPing = std::make_shared<Timer>(eventLoop);

  // Send ping every second (sending a high priority package restarts the timer)
  autoPing->start(1000, [this]() { sendPing(); }, true);
}

void Transmitter::setRttCallback(RttCallback callback)
{
  onRtt = callback;
}

void Transmitter::setResendTimeoutCallback(ResendTimeoutCallback callback)
{
  onResendTimeout = callback;
}

void Transmitter::setResentPacketsCallback(ResentPacketsCallback callback)
{
  onResentPackets = callback;
}

void Transmitter::setVideoCallback(VideoCallback callback)
{
  onVideo = callback;
}

void Transmitter::setAudioCallback(AudioCallback callback)
{
  onAudio = callback;
}

void Transmitter::setDebugCallback(DebugCallback callback)
{
  onDebug = callback;
}

void Transmitter::setValueCallback(ValueCallback callback)
{
  onValue = callback;
}

void Transmitter::setPeriodicValueCallback(PeriodicValueCallback callback)
{
  onPeriodicValue = callback;
}

void Transmitter::setNetworkRateCallback(NetworkRateCallback callback)
{
  onNetworkRate = callback;
}

void Transmitter::setConnectionStatusCallback(ConnectionStatusCallback callback)
{
  onConnectionStatus = callback;
}

void Transmitter::sendPing()
{
  std::cout << "Sending ping" << std::endl;

  auto msg = new Message(MessageType::Ping);
  sendMessage(msg);
}

void Transmitter::sendVideo(std::vector<std::uint8_t>* video, std::uint8_t index)
{
  std::cout << "Sending video" << std::endl;

  auto msg = new Message(MessageType::Video, index);

  // Append media payload
  msg->data()->insert(msg->data()->end(), video->begin(), video->end());

  // Clean up input data
  delete video;

  sendMessage(msg);
}

void Transmitter::sendAudio(std::vector<std::uint8_t>* audio)
{
  std::cout << "Sending audio" << std::endl;

  auto msg = new Message(MessageType::Audio);

  // Append media payload
  msg->data()->insert(msg->data()->end(), audio->begin(), audio->end());

  // Clean up input data
  delete audio;

  sendMessage(msg);
}

void Transmitter::sendDebug(std::string* debug)
{
  std::cout << "Sending debug message" << std::endl;

  auto msg = new Message(MessageType::Debug);

  // Truncate if necessary
  if (debug->length() > MSG_DEBUG_MAX_LEN) {
    debug->resize(MSG_DEBUG_MAX_LEN);
  }

  // Append debug payload (convert std::string to bytes)
  msg->data()->insert(msg->data()->end(), debug->begin(), debug->end());

  // Clean up input data
  delete debug;

  sendMessage(msg);
}

void Transmitter::sendValue(std::uint8_t subType, std::uint16_t value)
{
  std::cout << "Sending value: type=" << Message::getSubTypeStr(subType)
            << ", value=" << value << std::endl;

  auto msg = new Message(MessageType::Value, subType);
  msg->setPayload16(value);
  sendMessage(msg);
}

void Transmitter::sendPeriodicValue(std::uint8_t subType, std::uint16_t value)
{
  std::cout << "Sending periodic value: type=" << Message::getSubTypeStr(subType)
            << ", value=" << value << std::endl;

  auto msg = new Message(MessageType::PeriodicValue, subType);
  msg->setPayload16(value);
  sendMessage(msg);
}

void Transmitter::sendMessage(Message* msg)
{
  msg->setCRC();

  printData(msg->data());

  // Resolve the remote endpoint if needed
  if (remote_endpoint.address().is_unspecified()) {
    asio::ip::udp::resolver resolver(eventLoop.context());
    asio::error_code ec;
    auto endpoints = resolver.resolve(asio::ip::udp::v4(), relayHost, std::to_string(relayPort), ec);
    if (ec) {
      std::cerr << "Failed to resolve remote endpoint: " << ec.message() << std::endl;
      delete msg;
      return;
    }
    remote_endpoint = *endpoints.begin();
  }

  // Send the datagram
  socket.async_send_to(
    asio::buffer(*msg->data()),
    remote_endpoint,
    [this, msg](const asio::error_code& error, std::size_t bytes_transferred) {
      if (error) {
        std::cerr << "Failed to send datagram: " << error.message() << std::endl;
      } else {
        payloadSent += bytes_transferred;
        totalSent += bytes_transferred + 28; // UDP + IPv4 headers

        // Reset auto ping timer if sending High Prio (or ack) packet (unless sending a ping)
        if (autoPing && (msg->isHighPriority() || msg->type() == MessageType::Ack) && msg->type() != MessageType::Ping) {
          autoPing->start(1000, [this]() { sendPing(); }, true);
        }

        // If not a high priority package, all is done
        if (!msg->isHighPriority()) {
          delete msg;
          return;
        }

        // Start connection timeout timer
        startConnectionTimeout();

        // Store pointer to message until it's acked
        std::uint16_t fullType = msg->fullType();
        resendMessages[fullType].reset(msg);

        // Start high priority package resend
        startResendTimer(msg);

        // Start (or restart) round trip timer
        startRTTimer(msg);
      }
    });
}

void Transmitter::resendMessage(Message* msg)
{
  std::uint16_t fullType = msg->fullType();

  if (!resendMessages[fullType]) {
    std::cerr << "Warning: No message to resend for type " << fullType << std::endl;
    return;
  }

  resendCounter++;
  if (onResentPackets) {
    onResentPackets(resendCounter);
  }

  if (connectionStatus == CONNECTION_STATUS_OK) {
    connectionStatus = CONNECTION_STATUS_RETRYING;
    if (onConnectionStatus) {
      onConnectionStatus(connectionStatus);
    }
  }

  // Create a copy of the message and send it
  Message* resendMsg = new Message(*msg->data());
  sendMessage(resendMsg);
}

void Transmitter::startResendTimer(Message* msg)
{
  std::uint16_t fullType = msg->fullType();

  // Create a new timer if needed
  if (!resendTimers[fullType]) {
    resendTimers[fullType] = std::make_shared<Timer>(eventLoop);
  } else {
    resendTimers[fullType]->stop();
  }

  // Start the timer with reference to the stored message
  resendTimers[fullType]->start(resendTimeoutMs, [this, msg]() {
    resendMessage(msg);
  });
}

void Transmitter::startRTTimer(Message* msg)
{
  std::uint16_t fullType = msg->fullType();

  // Start or restart the round-trip timer
  rtTimers[fullType] = std::make_shared<std::chrono::steady_clock::time_point>(
    std::chrono::steady_clock::now());
}

void Transmitter::startConnectionTimeout()
{
  // Start existing timer (if inactive), or create a new one
  if (!connectionTimeoutTimer) {
    connectionTimeoutTimer = std::make_shared<Timer>(eventLoop);
  } else if (connectionTimeoutTimer) {
    connectionTimeoutTimer->stop();
  }

  // FIXME: 4 * resendTimeoutMs but never less than e.g. 2 secs?
  connectionTimeoutTimer->start(4 * resendTimeoutMs, [this]() {
    connectionTimeout();
  });
}

void Transmitter::readPendingDatagrams()
{
  // Prepare buffer for incoming data
  std::vector<std::uint8_t> buffer(4096); // Adjust size as needed

  // Start async receive
  socket.async_receive_from(
    asio::buffer(buffer),
    remote_endpoint,
    [this, buffer = std::move(buffer)](const asio::error_code& error, std::size_t bytes_transferred) mutable {
      if (!running) {
        return; // Exit if we're shutting down
      }

      if (!error) {
        std::cout << "Received datagram" << std::endl;

        // Resize buffer to actual data received
        buffer.resize(bytes_transferred);

        payloadRecv += bytes_transferred;
        totalRecv += bytes_transferred + 28; // UDP + IPv4 headers

        std::cout << "Sender: " << remote_endpoint.address().to_string()
                  << ", port: " << remote_endpoint.port() << std::endl;

        printData(&buffer);
        parseData(&buffer);
      } else if (error != asio::error::operation_aborted) {
        std::cerr << "Error receiving datagram: " << error.message() << std::endl;
      }

      // Continue reading
      if (running) {
        readPendingDatagrams();
      }
    });
}

void Transmitter::printError(int error)
{
  std::cerr << "Socket error (" << error << ")" << std::endl;
}

void Transmitter::printData(std::vector<std::uint8_t>* data)
{
  std::cout << "Data length: " << data->size() << std::endl;

  if (data->size() > 32) {
    std::cout << "Big packet (video?), not printing content" << std::endl;
  } else {
    // Print in hex format
    std::cout << "Data: ";
    for (auto byte : *data) {
      std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
  }
}

void Transmitter::parseData(std::vector<std::uint8_t>* data)
{
  std::cout << "Parsing received data" << std::endl;

  Message msg(*data);

  // isValid() also checks that the packet is exactly as long as expected
  if (!msg.isValid()) {
    std::cerr << "Package not valid, ignoring" << std::endl;
    return;
  }

  std::cout << "Received message type: " << Message::getTypeStr(msg.type()) << std::endl;

  // New data -> connection ok
  if (connectionStatus != CONNECTION_STATUS_OK) {
    connectionStatus = CONNECTION_STATUS_OK;
    if (onConnectionStatus) {
      onConnectionStatus(connectionStatus);
    }
  }

  // Stop connection timeout timer as we got something from the slave
  if (connectionTimeoutTimer) {
    connectionTimeoutTimer->stop();
  }

  // Check whether to ACK the packet
  if (msg.isHighPriority()) {
    sendACK(msg);
  }

  // Handle different message types in different methods
  if (messageHandlers[msg.type()]) {
    messageHandler func = messageHandlers[msg.type()];
    (this->*func)(msg);
  } else {
    std::cerr << "No message handler for type "
              << Message::getTypeStr(msg.type()) << ", ignoring" << std::endl;
  }
}

void Transmitter::sendACK(Message &incoming)
{
  std::cout << "Sending ACK" << std::endl;

  auto msg = new Message(MessageType::Ack);

  // Sets CRC as well
  msg->setACK(incoming);

  sendMessage(msg);
}

void Transmitter::handleACK(Message &msg)
{
  std::cout << "Handling ACK" << std::endl;

  std::uint16_t ackedFullType = msg.getAckedFullType();
  std::uint16_t ackedCRC = msg.getAckedCRC();

  // If the ack is not for the latest msg, ignore it
  if (resendMessages[ackedFullType] &&
      !resendMessages[ackedFullType]->matchCRC(ackedCRC)) {
    // We got ack, just not for the latest package. Restart timer to avoid continuous resends.
    if (resendTimers[ackedFullType]) {
      resendTimers[ackedFullType]->start(resendTimeoutMs, [this, msg = resendMessages[ackedFullType].get()]() {
        resendMessage(msg);
      });
    }
    std::cout << "Acked CRC does not match for type: " << ackedFullType << std::endl;
    return;
  }

  // Stop resend timer
  if (resendTimers[ackedFullType]) {
    resendTimers[ackedFullType]->stop();
    resendTimers[ackedFullType].reset();
  } else {
    std::cerr << "No Resend timer running for type " << ackedFullType << std::endl;
  }

  // Process RTT and adjust resend timeout
  if (rtTimers[ackedFullType]) {
    auto now = std::chrono::steady_clock::now();
    auto start = *rtTimers[ackedFullType];
    int rttMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

    // Emit RTT callback
    if (onRtt) {
      onRtt(rttMs);
    }

    // Adjust resend timeout but keep it always > 20ms.
    // If the doubled round trip time is less than current timeout, decrease resendTimeoutMs by 10%.
    // if the doubled round trip time is greater that current resendTimeoutMs, increase resendTimeoutMs to 2x rtt
    if (2 * rttMs < resendTimeoutMs) {
      resendTimeoutMs -= (int)(0.1 * resendTimeoutMs);
    } else {
      resendTimeoutMs = 2 * rttMs;
    }

    if (resendTimeoutMs < 20) {
      resendTimeoutMs = 20;
    }

    // Emit timeout callback
    if (onResendTimeout) {
      onResendTimeout(resendTimeoutMs);
    }

    std::cout << "New resend timeout: " << resendTimeoutMs << std::endl;

    rtTimers[ackedFullType].reset();
  } else {
    std::cerr << "No RT timer running for type " << ackedFullType << std::endl;
  }

  // Delete the message waiting for resend
  if (resendMessages[ackedFullType]) {
    resendMessages[ackedFullType].reset();
  }
}

void Transmitter::handlePing(Message &)
{
  std::cout << "Handling ping" << std::endl;
  // We don't do anything with ping (ACKing it is enough).
  // This handler is here to avoid missing handler warning.
}

void Transmitter::handleVideo(Message &msg)
{
  std::cout << "Handling video" << std::endl;

  // Copy the data and remove header to get the actual video payload
  auto* data = new std::vector<std::uint8_t>(*msg.data());
  data->erase(data->begin(), data->begin() + MessageOffset::Payload);

  // Send the received video payload to the application via callback
  if (onVideo) {
    onVideo(data, msg.subType());
  } else {
    delete data; // Clean up if no callback
  }
}

void Transmitter::handleAudio(Message &msg)
{
  std::cout << "Handling audio" << std::endl;

  // Copy the data and remove header to get the actual audio payload
  auto* data = new std::vector<std::uint8_t>(*msg.data());
  data->erase(data->begin(), data->begin() + MessageOffset::Payload);

  // Send the received audio payload to the application via callback
  if (onAudio) {
    onAudio(data);
  } else {
    delete data; // Clean up if no callback
  }
}

void Transmitter::handleDebug(Message &msg)
{
  std::cout << "Handling debug" << std::endl;

  // Extract debug payload
  std::vector<std::uint8_t> data(*msg.data());
  data.erase(data.begin(), data.begin() + MessageOffset::Payload);

  // Convert to string
  auto* debug = new std::string(data.begin(), data.end());

  // Send the received debug message to the application via callback
  if (onDebug) {
    onDebug(debug);
  } else {
    delete debug; // Clean up if no callback
  }
}

void Transmitter::handleValue(Message &msg)
{
  std::cout << "Handling value" << std::endl;

  std::uint8_t type = msg.subType();
  std::uint16_t val = msg.getPayload16();

  // Emit the callback with value
  if (onValue) {
    onValue(type, val);
  }
}

void Transmitter::handlePeriodicValue(Message &msg)
{
  std::cout << "Handling periodic value" << std::endl;

  std::uint8_t type = msg.subType();
  std::uint16_t val = msg.getPayload16();

  // Emit the callback with periodic value
  if (onPeriodicValue) {
    onPeriodicValue(type, val);
  }
}

void Transmitter::updateRate()
{
  // Time in ms since last update
  auto now = std::chrono::steady_clock::now();
  auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - rateTime).count();
  rateTime = now;

  if (elapsedMs == 0) {
    return; // Avoid division by zero
  }

  // Bytes received per second on average since last update
  int payloadRx = static_cast<int>(payloadRecv * 1000.0 / elapsedMs);
  payloadRecv = 0;
  int totalRx = static_cast<int>(totalRecv * 1000.0 / elapsedMs);
  totalRecv = 0;

  // Bytes sent per second on average since last update
  int payloadTx = static_cast<int>(payloadSent * 1000.0 / elapsedMs);
  payloadSent = 0;
  int totalTx = static_cast<int>(totalSent * 1000.0 / elapsedMs);
  totalSent = 0;

  // Emit rate callback
  if (onNetworkRate) {
    onNetworkRate(payloadRx, totalRx, payloadTx, totalTx);
  }
}

void Transmitter::connectionTimeout()
{
  std::cout << "Connection timeout" << std::endl;

  // Reset timeout to default
  resendTimeoutMs = RESEND_TIMEOUT_DEFAULT;

  if (connectionStatus != CONNECTION_STATUS_LOST) {
    connectionStatus = CONNECTION_STATUS_LOST;

    // Emit connection status callback
    if (onConnectionStatus) {
      onConnectionStatus(connectionStatus);
    }
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
