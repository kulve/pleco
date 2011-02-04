
#include "Message.h"

#include <QDebug>

#include <stdint.h>

Message::Message(QByteArray data):
  bytearray(data), msgType(MSG_TYPE_NONE), msgCRC(0)
{
  qDebug() << "in" << __FUNCTION__;

  // Initialize type array for casting int -> enum
  // FIXME: is this really needed? Can't cast uint8_t -> MSG_TYPE?
  for (int i = 0; i < MSG_TYPE_MAX; ++i) {
	types[i] = MSG_TYPE_NONE;
  }
  types[MSG_TYPE_NONE]     = MSG_TYPE_NONE;
  types[MSG_TYPE_PING]     = MSG_TYPE_PING;
  types[MSG_TYPE_C_A_S]    = MSG_TYPE_C_A_S;
  types[MSG_TYPE_STATS]    = MSG_TYPE_STATS;
  types[MSG_TYPE_ACK]      = MSG_TYPE_ACK;

  msgCRC = getCRC(bytearray); // Get CRC from the msg data, don't calculate it
  msgType = types[(uint8_t)bytearray.at(TYPE_OFFSET_TYPE)]; 

  qDebug() << __FUNCTION__ << ": Created a package with type " << msgType << "/" << (uint8_t)msgType << "(" << (uint8_t)bytearray.at(TYPE_OFFSET_TYPE) << ")";
}



Message::Message(MSG_TYPE type):
  bytearray(), msgType(type), msgCRC(0)
{
  qDebug() << "in" << __FUNCTION__;

  bytearray.resize(length(type));
  if (bytearray.size() == 0) {
	return; // Invalid message
  }

  bytearray.fill(0); // Zero data

  bytearray[TYPE_OFFSET_TYPE] = (uint8_t)type;
  
  setCRC();

  qDebug() << __FUNCTION__ << ": Created a package with type " << msgType << "/" << (uint8_t)msgType << "(" << (uint8_t)bytearray.at(TYPE_OFFSET_TYPE) << ")";
}



void Message::setACK(Message &msg)
{

  MSG_TYPE type = msg.type();

  // Make sure the size matches ACK message
  bytearray.resize(length(MSG_TYPE_ACK));

  bytearray.fill(0); // Zero data

  bytearray[TYPE_OFFSET_TYPE] = (uint8_t)MSG_TYPE_ACK;

  // Set the type we are acking
  bytearray[TYPE_OFFSET_PAYLOAD] = (uint8_t)type;

  // Set the CRC of the message we are acking
  bytearray[TYPE_OFFSET_PAYLOAD + 1] = (uint8_t)(msg.data()->at(TYPE_OFFSET_CRC + 0));
  bytearray[TYPE_OFFSET_PAYLOAD + 2] = (uint8_t)(msg.data()->at(TYPE_OFFSET_CRC + 1));

  setCRC(); 
}



Message::MSG_TYPE Message::getAckedType(void)
{

  // Send none as the type if the packet is not ACK packet
  if (msgType != MSG_TYPE_ACK) {
	return MSG_TYPE_NONE;
  }

  // Send none as the type if the packet is too short
  if (bytearray.size() < length(MSG_TYPE_ACK)) {
	return MSG_TYPE_NONE;
  }

  // return the acked type
  return (MSG_TYPE)(bytearray.at(TYPE_OFFSET_PAYLOAD));
}



Message::~Message()
{
  qDebug() << "in" << __FUNCTION__;
}



bool Message::isValid(void)
{

  if (bytearray.size() < TYPE_OFFSET_PAYLOAD) {
	qWarning() << "Invalid message length:" << bytearray.size() << ", discarding";
	return false;
  }


  if (bytearray.size() != length(msgType)) {
	qWarning() << "Invalid message length (" << bytearray.size() << ") for type" << msgType <<  ", discarding";
	return false;
  }
  
  return validateCRC();
}



bool Message::isHighPriority(void)
{
  // Package is considered a high priority, if the type < MSG_HP_TYPE_LIMIT
  return msgType < MSG_HP_TYPE_LIMIT;
}



Message::MSG_TYPE Message::type(void)
{
  qDebug() << "in" << __FUNCTION__;
  return msgType;
}


int Message::length(void)
{
  qDebug() << "in" << __FUNCTION__;

  return length(msgType);
}



int Message::length(MSG_TYPE type)
{
  qDebug() << "in" << __FUNCTION__;

  switch(type) {
  case MSG_TYPE_PING:
	return TYPE_OFFSET_PAYLOAD + 0; // no payload
  case MSG_TYPE_C_A_S:
	return TYPE_OFFSET_PAYLOAD + 4; // + CAMERA X + Y + MOTOR RIGHT + LEFT
  case MSG_TYPE_STATS:
	return TYPE_OFFSET_PAYLOAD + 3; // + UPTIME + LOAD AVG + WLAN
  case MSG_TYPE_ACK:
	return TYPE_OFFSET_PAYLOAD + 3; // + type + 16 bit CRC
  default:
	qWarning() << "Message length for type" << type << "not known";
	return 0;
  }
}



QByteArray *Message::data(void)
{
  qDebug() << "in" << __FUNCTION__;
  return &bytearray;
}



void Message::setCRC(void)
{
  // Zero CRC field in data before calculating new CRC
  bytearray.data()[TYPE_OFFSET_CRC + 0] = 0;
  bytearray.data()[TYPE_OFFSET_CRC + 1] = 0;

  // Calculate CRC
  msgCRC = qChecksum(bytearray.data(), bytearray.size());

  // Set CRC
  *((quint16 *)(bytearray.data())) = msgCRC;
}



quint16 Message::getCRC(QByteArray &data)
{

  quint16 crc = *((quint16 *)(data.data()));

  return crc;
}




bool Message::validateCRC(void)
{

  // FIXME: would the CRC be unchanged if we start from index 2
  // compared to case where we start from index 0 with two zeros in
  // front?

  // Zero CRC field in data before calculating new CRC
  bytearray.data()[TYPE_OFFSET_CRC + 0] = 0;
  bytearray.data()[TYPE_OFFSET_CRC + 1] = 0;

  // Calculate CRC
  quint16 calculated = qChecksum(bytearray.data(), bytearray.size());

  // Set old CRC back
  *((quint16 *)(bytearray.data())) = msgCRC;

  bool isValid = (msgCRC == calculated);

  return isValid;
}
