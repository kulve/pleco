
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

  msgCRC = bytearray[0];
  msgType = types[(uint8_t)bytearray.at(1)]; 

  qDebug() << __FUNCTION__ << ": Created a package with type " << msgType << "/" << (uint8_t)msgType << "(" << (uint8_t)bytearray.at(1) << ")";
}



Message::Message(MSG_TYPE type):
  bytearray(), msgType(type), msgCRC(0)
{
  qDebug() << "in" << __FUNCTION__;

  bytearray.resize(length(type));
  if (bytearray.size() == 0) {
	return; // Invalid message
  }

  bytearray[0] = 0; // TODO: calculate CRC (qChecksum?)
  bytearray[1] = (uint8_t)type;

  qDebug() << __FUNCTION__ << ": Created a package with type " << msgType << "/" << (uint8_t)msgType << "(" << (uint8_t)bytearray.at(1) << ")";
}



void Message::setACK(MSG_TYPE type)
{

  // Make sure the size matches ACK message
  bytearray.resize(length(MSG_TYPE_ACK));

  bytearray[0] = 0; // TODO: calculate CRC (qChecksum?)
  bytearray[1] = (uint8_t)MSG_TYPE_ACK;
  bytearray[2] = (uint8_t)type;
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
  return (MSG_TYPE)(bytearray.at(2));
}



Message::~Message()
{
  qDebug() << "in" << __FUNCTION__;
}



bool Message::isValid(void)
{

  if (bytearray.size() < 2) {
	qWarning() << "Invalid message length:" << bytearray.size() << ", discarding";
	return false;
  }


  if (bytearray.size() != length(msgType)) {
	qWarning() << "Invalid message length (" << bytearray.size() << ") for type" << msgType <<  ", discarding";
	return false;
  }
  
  // TODO: validate CRC (qChecksum?)
  return true;
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
	return 2; // CRC + ping
  case MSG_TYPE_C_A_S:
	return 6; // CRC + C_A_S + CAMERA X + Y + MOTOR RIGHT + LEFT
  case MSG_TYPE_STATS:
	return 5; // CRC + STATS + UPTIME + LOAD AVG + WLAN
  case MSG_TYPE_ACK:
	return 3; // CRC + ACK + type
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
