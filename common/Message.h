#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>

#define MSG_HP_TYPE_LIMIT            64   // Initial guess


class Message : public QObject
{
  Q_OBJECT;

 public:

  // Message type < MSG_HP_TYPE_LIMIT are high priority and must be acknowledged
  typedef enum MSG_TYPE {
	/* Below are high priority packages */
	MSG_TYPE_NONE         =   0,
	MSG_TYPE_PING         =   1,
	MSG_TYPE_C_A_S        =   2,
	/* Below are low priority packages */
	MSG_TYPE_STATS        =  65,
	MSG_TYPE_ACK          = 255,
	MSG_TYPE_MAX          = 256
  } MSG_TYPE;

  // Type offsets in a message
  typedef enum TYPE_OFFSET {
	/* Below are high priority packages */
	TYPE_OFFSET_CRC       =   0,
	TYPE_OFFSET_TYPE      =   2,
	TYPE_OFFSET_PAYLOAD   =   3
  } TYPE_OFFSET;

  Message(QByteArray data);
  Message(MSG_TYPE type);
  ~Message();
  void setACK(Message &msg);
  MSG_TYPE getAckedType(void);
  MSG_TYPE type(void);
  bool isValid(void);
  bool isHighPriority(void);

  QByteArray *data(void);
  int length(void);
  int length(MSG_TYPE type);
  void setCRC(void);
  quint16 getCRC(QByteArray &data);
  bool validateCRC(void);

 private:

  QByteArray bytearray;
  MSG_TYPE msgType;
  quint16 msgCRC;
  MSG_TYPE types[MSG_TYPE_MAX];
};

#endif

