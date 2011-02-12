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
// Below are low priority packages 
#define MSG_TYPE_STATS               65
#define MSG_TYPE_ACK                255
#define MSG_TYPE_MAX                256

#define TYPE_OFFSET_CRC               0
#define TYPE_OFFSET_TYPE              2
#define TYPE_OFFSET_PAYLOAD           3


class Message : public QObject
{
  Q_OBJECT;

 public:

  Message(QByteArray data);
  Message(quint8 type);
  ~Message();
  void setACK(Message &msg);
  quint8 getAckedType(void);
  quint8 type(void);
  bool isValid(void);
  bool isHighPriority(void);

  QByteArray *data(void);
  void setCRC(void);
  bool validateCRC(void);

 private:
  int length(void);
  int length(quint8 type);
  quint16 getCRC(void);

  QByteArray bytearray;
};

#endif

