/*
 * Copyright 2015-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>


constexpr std::size_t MSG_DEBUG_MAX_LEN = 256U;      // Max length of debug messages

constexpr std::size_t MSG_HP_TYPE_LIMIT = 64U;       // Highest low-priority type

constexpr std::size_t MSG_TYPE_SUBTYPE_MAX = 65536;
constexpr std::size_t MSG_TYPE_MAX         = 256;

namespace MessageType {
  constexpr std::uint8_t None            = 0U;
  constexpr std::uint8_t Ping            = 1U;
  constexpr std::uint8_t Value           = 3U;
  // Below are low priority packages
  constexpr std::uint8_t Stats           = 65U;
  constexpr std::uint8_t Video           = 66U;
  constexpr std::uint8_t Audio           = 67U;
  constexpr std::uint8_t Debug           = 68U;
  constexpr std::uint8_t PeriodicValue   = 69U;
  constexpr std::uint8_t Ack             = 255U;
}


namespace MessageSubtype {
  constexpr std::uint16_t None              = 0U;
  constexpr std::uint16_t EnableLED         = 1U;
  constexpr std::uint16_t EnableVideo       = 2U;
  constexpr std::uint16_t EnableAudio       = 3U;
  constexpr std::uint16_t VideoSource       = 4U;
  constexpr std::uint16_t CameraXY          = 5U;
  constexpr std::uint16_t CameraZoom        = 6U;
  constexpr std::uint16_t CameraFocus       = 7U;
  constexpr std::uint16_t SpeedTurn         = 8U;
  constexpr std::uint16_t BatteryCurrent    = 9U;
  constexpr std::uint16_t BatteryVoltage    = 10U;
  constexpr std::uint16_t Distance          = 11U;
  constexpr std::uint16_t Temperature       = 12U;
  constexpr std::uint16_t SignalStrength    = 13U;
  constexpr std::uint16_t CPUUsage          = 14U;
  constexpr std::uint16_t VideoQuality      = 15U;
  constexpr std::uint16_t Uptime            = 16U;
}

namespace MessageOffset {
  constexpr std::size_t CRC            = 0;   // 16 bit CRC
  constexpr std::size_t Sequence       = 2;   // 16 bit sequence number
  constexpr std::size_t Type           = 4;   // 8 bit type
  constexpr std::size_t Subtype        = 5;   // 8 bit sub type
  constexpr std::size_t Payload        = 6;   // start of payload
  constexpr std::size_t AckedType      = 6;   // Acked 8 bit type
  constexpr std::size_t AckedSubtype   = 7;   // Acked 8 bit sub type
  constexpr std::size_t AckedCRC       = 8;   // Acked 16 bit CRC
}



class Message {
 public:
  Message(const std::vector<std::uint8_t>& data);
  Message(std::uint8_t type, std::uint8_t subType = 0);
  ~Message();

  void setACK(Message &msg);
  std::uint8_t getAckedType(void);
  std::uint8_t getAckedSubType(void);
  std::uint16_t getAckedFullType(void);
  std::uint16_t getAckedCRC(void);
  std::uint8_t type(void);
  std::uint8_t subType(void);
  std::uint16_t fullType(void);
  bool isValid(void);
  bool isHighPriority(void);

  std::vector<std::uint8_t>* data(void);
  void setCRC(void);
  bool validateCRC(void);
  bool matchCRC(std::uint16_t test);
  void setSeq(std::uint16_t seq);

  void setPayload16(std::uint16_t value);
  std::uint16_t getPayload16();

  static std::string getTypeStr(std::uint16_t type);
  static std::string getSubTypeStr(std::uint16_t type);

 private:
  std::size_t length(void);
  std::size_t length(std::uint8_t type);
  std::uint16_t getCRC(void);
  void setUint16(std::size_t index, std::uint16_t value);
  std::uint16_t getUint16(std::size_t index);

  std::vector<std::uint8_t> bytearray;
};

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
