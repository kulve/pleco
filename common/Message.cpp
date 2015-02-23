/*
 * Copyright 2015 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Message.h"

#include <QDebug>

#include <stdint.h>

static quint16 seqs[MSG_TYPE_SUBTYPE_MAX] = {0};

Message::Message(QByteArray data):
  bytearray(data)
{
  //qDebug() << "in" << __FUNCTION__;

  qDebug() << __FUNCTION__ << ": Created a message with type " << getTypeStr((quint8)bytearray.at(TYPE_OFFSET_TYPE))
		   << ", length: " << data.length();
}

Message::Message(quint8 type, quint8 subType):
  bytearray()
{
  //qDebug() << "in" << __FUNCTION__;

  bytearray.resize(length(type));
  if (bytearray.size() < TYPE_OFFSET_PAYLOAD) {
	return; // Invalid message, must have space at least for mandatory
			// data before payload
  }

  bytearray.fill(0); // Zero data

  bytearray[TYPE_OFFSET_TYPE] = type;
  bytearray[TYPE_OFFSET_SUBTYPE] = subType;

  setSeq(seqs[fullType()]++);
  
  setCRC();

  qDebug() << __FUNCTION__ << ": Created a message with type" << getTypeStr((quint8)bytearray.at(TYPE_OFFSET_TYPE))
		   << ", sub type" << getSubTypeStr((quint8)bytearray.at(TYPE_OFFSET_SUBTYPE));
}



Message::~Message()
{
  //qDebug() << "in" << __FUNCTION__;
}



void Message::setACK(Message &msg)
{

  quint8 type = msg.type();
  quint8 subType = msg.subType();

  Q_ASSERT(bytearray.size() >= length(MSG_TYPE_ACK));

  bytearray[TYPE_OFFSET_TYPE] = MSG_TYPE_ACK;

  // Set the type and possible subtype we are acking
  bytearray[TYPE_OFFSET_ACKED_TYPE]    = type;
  bytearray[TYPE_OFFSET_ACKED_SUBTYPE] = subType;

  // Copy the CRC of the message we are acking
  QByteArray *data = msg.data();
  bytearray[TYPE_OFFSET_ACKED_CRC + 0] = (*data)[TYPE_OFFSET_CRC + 0];
  bytearray[TYPE_OFFSET_ACKED_CRC + 1] = (*data)[TYPE_OFFSET_CRC + 1];

  setCRC(); 
}



quint8 Message::getAckedType(void)
{

  // Return none as the type if the packet is not ACK packet
  if (type() != MSG_TYPE_ACK) {
	return MSG_TYPE_NONE;
  }

  // Return none as the type if the packet is too short
  if (bytearray.size() < length(MSG_TYPE_ACK)) {
	return MSG_TYPE_NONE;
  }

  // return the acked type
  return bytearray.at(TYPE_OFFSET_PAYLOAD);
}



quint8 Message::getAckedSubType(void)
{

  // Return none as the type if the packet is not ACK packet
  if (type() != MSG_TYPE_ACK) {
	return MSG_TYPE_NONE;
  }

  // Return none as the type if the packet is too short
  if (bytearray.size() < length(MSG_TYPE_ACK)) {
	return MSG_TYPE_NONE;
  }

  // Return the acked sub type
  return bytearray.at(TYPE_OFFSET_PAYLOAD + 1);
}



quint16 Message::getAckedFullType(void)
{
  quint16 type = getAckedType();
  type <<= 8;
  type += getAckedSubType();

  return type;
}



quint16 Message::getAckedCRC(void)
{
  return getQuint16(TYPE_OFFSET_ACKED_CRC);
}



bool Message::isValid(void)
{
  // Size must be at least big enough to hold mandatory headers before payload
  if (bytearray.size() < TYPE_OFFSET_PAYLOAD) {
	qWarning() << "Invalid message length:" << bytearray.size() << ", discarding";
	return false;
  }

  // Size must be at least the minimum size for the type
  if (bytearray.size() < length(type())) {
	qWarning() << "Invalid message length (" << bytearray.size() << ") for type" << getTypeStr(type()) <<  ", discarding";
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



quint8 Message::type(void)
{
  //qDebug() << "in" << __FUNCTION__;

  return (quint8)bytearray.at(TYPE_OFFSET_TYPE);
}



quint8 Message::subType(void)
{
  //qDebug() << "in" << __FUNCTION__;

  return (quint8)bytearray.at(TYPE_OFFSET_SUBTYPE);
}



quint16 Message::fullType(void)
{
  quint16 fulltype = type();
  fulltype <<= 8;
  fulltype += subType();

  return fulltype;
}



int Message::length(void)
{
  //qDebug() << "in" << __FUNCTION__;

  return length(type());
}



int Message::length(quint8 type)
{
  //qDebug() << "in" << __FUNCTION__;

  switch(type) {
  case MSG_TYPE_PING:
	return TYPE_OFFSET_PAYLOAD + 0; // no payload
  case MSG_TYPE_MEDIA:
	return TYPE_OFFSET_PAYLOAD + 0; // + payload of arbitrary length
  case MSG_TYPE_DEBUG:
	return TYPE_OFFSET_PAYLOAD + 0; // + payload of arbitrary length
  case MSG_TYPE_VALUE:
	return TYPE_OFFSET_PAYLOAD + 2; // + 16 bit value
  case MSG_TYPE_PERIODIC_VALUE:
	return TYPE_OFFSET_PAYLOAD + 2; // + 16 bit value
  case MSG_TYPE_ACK:
	return TYPE_OFFSET_PAYLOAD + 4; // + type + sub type + 16 bit CRC
  default:
	qWarning() << "Message length for type" << getTypeStr(type) << "not known";
	return 0;
  }
}



QByteArray *Message::data(void)
{
  //qDebug() << "in" << __FUNCTION__;
  return &bytearray;
}



void Message::setCRC(void)
{
  // Zero CRC field in data before calculating new 16bit CRC
  setQuint16(TYPE_OFFSET_CRC, 0);

  // Calculate 16bit CRC
  quint16 crc = qChecksum(bytearray.data(), bytearray.size());

  // Set 16bit CRC
  setQuint16(TYPE_OFFSET_CRC, crc);
}



void Message::setSeq(quint16 seq)
{
  setQuint16(TYPE_OFFSET_SEQ, seq);
}



quint16 Message::getCRC(void)
{
  return getQuint16(TYPE_OFFSET_CRC);
}



bool Message::validateCRC(void)
{

  quint16 crc = getCRC();

  // Zero CRC field in data before calculating new CRC
  setQuint16(TYPE_OFFSET_CRC, 0);

  // Calculate CRC
  quint16 calculated = qChecksum(bytearray.data(), bytearray.size());

  // Set old CRC back
  setQuint16(TYPE_OFFSET_CRC, crc);

  bool isValid = (crc == calculated);

  if (!isValid) {
	qDebug() << __FUNCTION__ << ": Embdedded CRC:" << hex << crc << ", calculated CRC:" << hex << calculated;
  }

  return isValid;
}



bool Message::matchCRC(quint16 test)
{
  quint16 crc = getCRC();
  bool match = crc == test;

  if (!match) {
	qDebug() << __FUNCTION__ << ": Embdedded CRC:" << hex << crc << ", match CRC:" << hex << test;
  }

  return match;
}



void Message::setPayload16(quint16 value)
{
  setQuint16(TYPE_OFFSET_PAYLOAD, value);
}



quint16 Message::getPayload16(void)
{
  return getQuint16(TYPE_OFFSET_PAYLOAD);
}



QString Message::getTypeStr(quint16 type)
{
  switch (type) {
  case MSG_TYPE_NONE:
	return QString("NONE");
  case MSG_TYPE_PING:
	return QString("PING");
  case MSG_TYPE_VALUE:
	return QString("VALUE");
  case MSG_TYPE_MEDIA:
	return QString("MEDIA");
  case MSG_TYPE_DEBUG:
	return QString("DEBUG");
  case MSG_TYPE_PERIODIC_VALUE:
	return QString("PERIODIC_VALUE");
  case MSG_TYPE_ACK:
	return QString("ACK");
  default:
	return QString("UNKNOWN") + "(" +  QString::number(type) + ")";
  }
}



QString Message::getSubTypeStr(quint16 type)
{
  switch (type) {
  case MSG_SUBTYPE_NONE:
	return QString("NONE");
  case MSG_SUBTYPE_ENABLE_LED:
	return QString("ENABLE_LED");
  case MSG_SUBTYPE_ENABLE_VIDEO:
	return QString("ENABLED_VIDEO");
  case MSG_SUBTYPE_VIDEO_SOURCE:
	return QString("VIDEO_SOURCE");
  case MSG_SUBTYPE_CAMERA_XY:
	return QString("CAMERA_XY");
  case MSG_SUBTYPE_SPEED_TURN:
	return QString("SPEED_TURN");
  case MSG_SUBTYPE_BATTERY_CURRENT:
	return QString("BATTERY_CURRENT");
  case MSG_SUBTYPE_BATTERY_VOLTAGE:
	return QString("BATTERY_VOLTAGE");
  case MSG_SUBTYPE_DISTANCE:
	return QString("DISTANCE");
  case MSG_SUBTYPE_TEMPERATURE:
	return QString("TEMPERATURE");
  case MSG_SUBTYPE_SIGNAL_STRENGTH:
	return QString("SIGNAL_STRENGTH");
  case MSG_SUBTYPE_CPU_USAGE:
	return QString("CPU_USAGE");
  case MSG_SUBTYPE_ENABLE_HIGHBITRATE:
	return QString("ENABLE_HIGHBITRATE");
  case MSG_SUBTYPE_UPTIME:
	return QString("UPTIME");
  default:
	return QString("UNKNOWN") + "(" +  QString::number(type) + ")";
  }
}



void Message::setQuint16(int index, quint16 value)
{
  bytearray[index + 0] = (quint8)((value & 0xff00) >> 8);
  bytearray[index + 1] = (quint8)((value & 0x00ff) >> 0);
}



quint16 Message::getQuint16(int index)
{
  return (((quint16)bytearray.at(index)) << 8) +
	(quint8)bytearray.at(index + 1);
}



