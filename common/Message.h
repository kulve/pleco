#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>

#define MSG_HP_TYPE_LIMIT 64

class Message : public QObject
{
  Q_OBJECT;

 public:

  // Message type < MSG_HP_TYPE_LIMIT are high priority and must be acknowledged
  typedef enum MSG_TYPE {
	/* Below are high priority packages */
	MSG_TYPE_NONE         =   0,
	MSG_TYPE_PING         =   1,
	/* Below are low priority packages */
	MSG_TYPE_STATS        =  65,
	MSG_TYPE_ACK          = 255,
	MSG_TYPE_MAX          = 256
  } MSG_TYPE;

  Message(QByteArray data);
  Message(MSG_TYPE type);
  ~Message();
  void setACK(MSG_TYPE type);
  MSG_TYPE getAckedType(void);
  MSG_TYPE type(void);
  bool isValid(void);
  bool isHighPriority(void);

  QByteArray *data(void);
  int length(void);
  int length(MSG_TYPE type);

 private:
  QByteArray bytearray;
  MSG_TYPE msgType;
  int msgCRC;
  MSG_TYPE types[MSG_TYPE_MAX];
};

#endif

