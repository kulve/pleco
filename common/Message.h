/*
 * Copyright 2015 Tuomas Kulve, <tuomas@kulve.fi>
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
#define MSG_TYPE_VIDEO               66
#define MSG_TYPE_AUDIO               67
#define MSG_TYPE_DEBUG               68
#define MSG_TYPE_PERIODIC_VALUE      69
#define MSG_TYPE_ACK                255
#define MSG_TYPE_MAX                256
#define MSG_TYPE_SUBTYPE_MAX      65536    // 16 bit full types


// These subtypes are just for convenience of the API user. They are not used in Message class.
enum {
  MSG_SUBTYPE_NONE,
  MSG_SUBTYPE_ENABLE_LED,
  MSG_SUBTYPE_ENABLE_VIDEO,
  MSG_SUBTYPE_ENABLE_AUDIO,
  MSG_SUBTYPE_VIDEO_SOURCE,
  MSG_SUBTYPE_CAMERA_XY,
  MSG_SUBTYPE_CAMERA_ZOOM,
  MSG_SUBTYPE_CAMERA_FOCUS,
  MSG_SUBTYPE_SPEED_TURN,
  MSG_SUBTYPE_BATTERY_CURRENT,
  MSG_SUBTYPE_BATTERY_VOLTAGE,
  MSG_SUBTYPE_DISTANCE,
  MSG_SUBTYPE_TEMPERATURE,
  MSG_SUBTYPE_SIGNAL_STRENGTH,
  MSG_SUBTYPE_CPU_USAGE,
  MSG_SUBTYPE_VIDEO_QUALITY,
  MSG_SUBTYPE_UPTIME
};

// Byte offsets inside a message
#define TYPE_OFFSET_CRC               0    // 16 bit CRC
#define TYPE_OFFSET_SEQ               2    // 16 bit sequence number
#define TYPE_OFFSET_TYPE              4    // 8 bit type
#define TYPE_OFFSET_SUBTYPE           5    // 8 bit sub type
#define TYPE_OFFSET_PAYLOAD           6    // start of payload
#define TYPE_OFFSET_ACKED_TYPE        6    // Acked 8 bit type
#define TYPE_OFFSET_ACKED_SUBTYPE     7    // Acked 8 bit sub type
#define TYPE_OFFSET_ACKED_CRC         8    // Acked 16 bit CRC

// Max length of debug messages
#define MSG_DEBUG_MAX_LEN             256

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
  quint16 getAckedCRC(void);
  quint8 type(void);
  quint8 subType(void);
  quint16 fullType(void);
  bool isValid(void);
  bool isHighPriority(void);

  QByteArray *data(void);
  void setCRC(void);
  bool validateCRC(void);
  bool matchCRC(quint16 test);
  void setSeq(quint16 seq);

  void setPayload16(quint16 value);
  quint16 getPayload16();

  static QString getTypeStr(quint16 type);
  static QString getSubTypeStr(quint16 type);

 private:
  int length(void);
  int length(quint8 type);
  quint16 getCRC(void);
  void setQuint16(int index, quint16 value);
  quint16 getQuint16(int index);

  QByteArray bytearray;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
