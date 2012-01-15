/*
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
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

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <QObject>

#define MSG_HP_TYPE_LIMIT            64   // Initial guess

// Message type < MSG_HP_TYPE_LIMIT are high priority and must be acknowledged
// Below are high priority packages
#define MSG_TYPE_NONE                 0
#define MSG_TYPE_PING                 1
#define MSG_TYPE_VALUE                3
// Below are low priority packages 
#define MSG_TYPE_STATS               65
#define MSG_TYPE_MEDIA               66
#define MSG_TYPE_ACK                255
#define MSG_TYPE_MAX                256
#define MSG_TYPE_SUBTYPE_MAX      65536    // 16 bit full types


// These subtypes are just for convenience of the API user. They are not used in Message class.
#define MSG_SUBTYPE_NONE              0
#define MSG_SUBTYPE_ENABLE_VIDEO      1
#define MSG_SUBTYPE_VIDEO_SOURCE      2

// Byte offsets inside a message
#define TYPE_OFFSET_CRC               0    // 16 bit CRC
#define TYPE_OFFSET_TYPE              2    // 8 bit type
#define TYPE_OFFSET_SUBTYPE           3    // 8 bit sub type
#define TYPE_OFFSET_PAYLOAD           4    // start of payload


class Message : public QObject
{
  Q_OBJECT;

 public:

  Message(QByteArray data);
  Message(quint8 type, quint8 subType = 0);
  ~Message();
  void setACK(Message &msg);
  quint8 getAckedType(void);
  quint8 getAckedSubType(void);
  quint16 getAckedFullType(void);
  quint8 type(void);
  quint8 subType(void);
  quint16 fullType(void);
  bool isValid(void);
  bool isHighPriority(void);

  QByteArray *data(void);
  void setCRC(void);
  bool validateCRC(void);

  void setPayload16(quint16 value);

 private:
  int length(void);
  int length(quint8 type);
  quint16 getCRC(void);
  void setQuint16(int index, quint16 value);

  QByteArray bytearray;
};

#endif

