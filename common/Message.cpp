/*
 * Copyright 2015-2025 Tuomas Kulve, <tuomas@kulve.fi>
 * SPDX-License-Identifier: MIT
 */

#include "Message.h"

#include <iostream>
#include <cstring>

// Static array to hold sequence numbers
static std::uint16_t seqs[MSG_TYPE_SUBTYPE_MAX] = {0};

// Simple CRC-16 implementation to replace qChecksum
std::uint16_t calculateCRC16(const std::uint8_t* data, std::size_t length) {
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < length; i++) {
        crc ^= (std::uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

Message::Message(const std::vector<std::uint8_t>& data) :
    bytearray(data)
{
    std::cout << __func__ << ": Created a message with type "
              << getTypeStr(bytearray[MessageOffset::Type])
              << ", length: " << data.size() << std::endl;
}

Message::Message(std::uint8_t type, std::uint8_t subType) :
    bytearray()
{
    bytearray.resize(length(type));
    if (bytearray.size() < MessageOffset::Payload) {
        return; // Invalid message, must have space at least for mandatory
                // data before payload
    }

    std::fill(bytearray.begin(), bytearray.end(), 0); // Zero data

    bytearray[MessageOffset::Type] = type;
    bytearray[MessageOffset::Subtype] = subType;

    setSeq(seqs[fullType()]++);

    setCRC();

    std::cout << __func__ << ": Created a message with type "
              << getTypeStr(bytearray[MessageOffset::Type])
              << ", sub type " << getSubTypeStr(bytearray[MessageOffset::Subtype]) << std::endl;
}

Message::~Message()
{
    // Nothing to do
}

void Message::setACK(Message &msg)
{
    std::uint8_t type = msg.type();
    std::uint8_t subType = msg.subType();

    // Assert removed - add explicit check instead
    if (bytearray.size() < length(MessageType::Ack)) {
        std::cerr << "Error: Buffer too small for ACK message" << std::endl;
        return;
    }

    bytearray[MessageOffset::Type] = MessageType::Ack;

    // Set the type and possible subtype we are acking
    bytearray[MessageOffset::AckedType] = type;
    bytearray[MessageOffset::AckedSubtype] = subType;

    // Copy the CRC of the message we are acking
    std::vector<std::uint8_t> *data = msg.data();
    bytearray[MessageOffset::AckedCRC + 0] = (*data)[MessageOffset::CRC + 0];
    bytearray[MessageOffset::AckedCRC + 1] = (*data)[MessageOffset::CRC + 1];

    setCRC();
}

std::uint8_t Message::getAckedType(void)
{
    // Return none as the type if the packet is not ACK packet
    if (type() != MessageType::Ack) {
        return MessageType::None;
    }

    // Return none as the type if the packet is too short
    if (bytearray.size() < length(MessageType::Ack)) {
        return MessageType::None;
    }

    // return the acked type
    return bytearray[MessageOffset::AckedType];
}

std::uint8_t Message::getAckedSubType(void)
{
    // Return none as the type if the packet is not ACK packet
    if (type() != MessageType::Ack) {
        return MessageSubtype::None;
    }

    // Return none as the type if the packet is too short
    if (bytearray.size() < length(MessageType::Ack)) {
        return MessageSubtype::None;
    }

    // Return the acked sub type
    return bytearray[MessageOffset::AckedSubtype];
}

std::uint16_t Message::getAckedFullType(void)
{
    std::uint16_t type = getAckedType();
    type <<= 8;
    type += getAckedSubType();

    return type;
}

std::uint16_t Message::getAckedCRC(void)
{
    return getUint16(MessageOffset::AckedCRC);
}

bool Message::isValid(void)
{
    // Size must be at least big enough to hold mandatory headers before payload
    if (bytearray.size() < MessageOffset::Payload) {
        std::cerr << "Invalid message length: " << bytearray.size() << ", discarding" << std::endl;
        return false;
    }

    // Size must be at least the minimum size for the type
    if (bytearray.size() < length(type())) {
        std::cerr << "Invalid message length (" << bytearray.size() << ") for type "
                 << getTypeStr(type()) << ", discarding" << std::endl;
        return false;
    }

    // CRC inside the message must match the calculated CRC
    return validateCRC();
}

bool Message::isHighPriority(void)
{
    // Message is considered a high priority, if the type < MSG_HP_TYPE_LIMIT
    return type() < MSG_HP_TYPE_LIMIT;
}

std::uint8_t Message::type(void)
{
    return bytearray[MessageOffset::Type];
}

std::uint8_t Message::subType(void)
{
    return bytearray[MessageOffset::Subtype];
}

std::uint16_t Message::fullType(void)
{
    std::uint16_t fulltype = type();
    fulltype <<= 8;
    fulltype += subType();

    return fulltype;
}

std::size_t Message::length(void)
{
    return length(type());
}

std::size_t Message::length(std::uint8_t type)
{
    switch(type) {
    case MessageType::Ping:
        return MessageOffset::Payload + 0; // no payload
    case MessageType::Video:
        return MessageOffset::Payload + 0; // + payload of arbitrary length
    case MessageType::Audio:
        return MessageOffset::Payload + 0; // + payload of arbitrary length
    case MessageType::Debug:
        return MessageOffset::Payload + 0; // + payload of arbitrary length
    case MessageType::Value:
        return MessageOffset::Payload + 2; // + 16 bit value
    case MessageType::PeriodicValue:
        return MessageOffset::Payload + 2; // + 16 bit value
    case MessageType::Ack:
        return MessageOffset::Payload + 4; // + type + sub type + 16 bit CRC
    default:
        std::cerr << "Message length for type " << getTypeStr(type) << " not known" << std::endl;
        return 0;
    }
}

std::vector<std::uint8_t>* Message::data(void)
{
    return &bytearray;
}

void Message::setCRC(void)
{
    // Zero CRC field in data before calculating new 16bit CRC
    setUint16(MessageOffset::CRC, 0);

    // Calculate 16bit CRC
    std::uint16_t crc = calculateCRC16(bytearray.data(), bytearray.size());

    // Set 16bit CRC
    setUint16(MessageOffset::CRC, crc);
}

void Message::setSeq(std::uint16_t seq)
{
    setUint16(MessageOffset::Sequence, seq);
}

std::uint16_t Message::getCRC(void)
{
    return getUint16(MessageOffset::CRC);
}

bool Message::validateCRC(void)
{
    std::uint16_t crc = getCRC();

    // Zero CRC field in data before calculating new CRC
    setUint16(MessageOffset::CRC, 0);

    // Calculate CRC
    std::uint16_t calculated = calculateCRC16(bytearray.data(), bytearray.size());

    // Set old CRC back
    setUint16(MessageOffset::CRC, crc);

    bool isValid = (crc == calculated);

    if (!isValid) {
        std::cout << __func__ << ": Embedded CRC: 0x" << std::hex << crc
                 << ", calculated CRC: 0x" << calculated << std::dec << std::endl;
    }

    return isValid;
}

bool Message::matchCRC(std::uint16_t test)
{
    std::uint16_t crc = getCRC();
    bool match = crc == test;

    if (!match) {
        std::cout << __func__ << ": Embedded CRC: 0x" << std::hex << crc
                 << ", match CRC: 0x" << test << std::dec << std::endl;
    }

    return match;
}

void Message::setPayload16(std::uint16_t value)
{
    setUint16(MessageOffset::Payload, value);
}

std::uint16_t Message::getPayload16(void)
{
    return getUint16(MessageOffset::Payload);
}

std::string Message::getTypeStr(std::uint16_t type)
{
    switch (type) {
    case MessageType::None:
        return "NONE";
    case MessageType::Ping:
        return "PING";
    case MessageType::Value:
        return "VALUE";
    case MessageType::Video:
        return "VIDEO";
    case MessageType::Audio:
        return "AUDIO";
    case MessageType::Debug:
        return "DEBUG";
    case MessageType::PeriodicValue:
        return "PERIODIC_VALUE";
    case MessageType::Ack:
        return "ACK";
    default:
        return "UNKNOWN(" + std::to_string(type) + ")";
    }
}

std::string Message::getSubTypeStr(std::uint16_t type)
{
    switch (type) {
    case MessageSubtype::None:
        return "NONE";
    case MessageSubtype::EnableLED:
        return "ENABLE_LED";
    case MessageSubtype::EnableVideo:
        return "ENABLED_VIDEO";
    case MessageSubtype::EnableAudio:
        return "ENABLED_AUDIO";
    case MessageSubtype::VideoSource:
        return "VIDEO_SOURCE";
    case MessageSubtype::CameraXY:
        return "CAMERA_XY";
    case MessageSubtype::CameraZoom:
        return "CAMERA_ZOOM";
    case MessageSubtype::CameraFocus:
        return "CAMERA_FOCUS";
    case MessageSubtype::SpeedTurn:
        return "SPEED_TURN";
    case MessageSubtype::BatteryCurrent:
        return "BATTERY_CURRENT";
    case MessageSubtype::BatteryVoltage:
        return "BATTERY_VOLTAGE";
    case MessageSubtype::Distance:
        return "DISTANCE";
    case MessageSubtype::Temperature:
        return "TEMPERATURE";
    case MessageSubtype::SignalStrength:
        return "SIGNAL_STRENGTH";
    case MessageSubtype::CPUUsage:
        return "CPU_USAGE";
    case MessageSubtype::VideoQuality:
        return "VIDEO_QUALITY";
    case MessageSubtype::Uptime:
        return "UPTIME";
    default:
        return "UNKNOWN(" + std::to_string(type) + ")";
    }
}

void Message::setUint16(std::size_t index, std::uint16_t value)
{
    bytearray[index + 0] = (std::uint8_t)((value & 0xff00) >> 8);
    bytearray[index + 1] = (std::uint8_t)((value & 0x00ff) >> 0);
}

std::uint16_t Message::getUint16(std::size_t index)
{
    return (((std::uint16_t)bytearray[index]) << 8) +
        (std::uint8_t)bytearray[index + 1];
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
