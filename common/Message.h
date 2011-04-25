#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>

#define MSG_HP_TYPE_LIMIT            64   // Initial guess

// Message type < MSG_HP_TYPE_LIMIT are high priority and must be acknowledged
// Below are high priority packages
#define MSG_TYPE_NONE                 0
#define MSG_TYPE_PING                 1
#define MSG_TYPE_C_A_S                2
#define MSG_TYPE_MEDIA                3
#define MSG_TYPE_VALUE                4
// Below are low priority packages 
#define MSG_TYPE_STATS               65
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

