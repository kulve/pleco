#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "Message.h"

#include <QtNetwork>
#include <QObject>

// Status messages for API user convenience. Not used in Transmitter.
// Bits of quint8
#define STATUS_VIDEO_ENABLED      0x1

class Transmitter : public QObject
{
  Q_OBJECT;

 public:

  // Message handler function prototype
  typedef void (Transmitter::*messageHandler)(Message &msg);

  Transmitter(QString host, quint16 port);
  ~Transmitter();
  void initSocket();
  void enableAutoPing(bool enable);

 public slots:
  void sendPing();
  void sendStats(QList <int> *stats);
  void sendCameraAndSpeed(int cameraX, int cameraY, int motorRight, int motorLeft);
  void sendMedia(QByteArray *media);
  void sendValue(quint8 type, quint16 value);

 private slots:
  void readPendingDatagrams();
  void printError(QAbstractSocket::SocketError error);
  void sendMessage(Message *msg);
  void sendMessage(QObject *msg);

 signals:
  void rtt(int ms);
  void resendTimeout(int ms);
  void resentPackets(quint32 resendCounter);
  void uptime(int seconds);
  void loadAvg(float avg);
  void wlan(int percent);
  void cameraX(int degrees);
  void cameraY(int degrees);
  void motorRight(int percent);
  void motorLeft(int percent);
  void media(QByteArray *media);
  void value(quint8 type, quint16 value);
  void status(quint8 status);

 private:
  void printData(QByteArray *data);
  void parseData(QByteArray *data);
  void handleACK(Message &msg);
  void handlePing(Message &msg);
  void handleStats(Message &msg);
  void handleCameraAndSpeed(Message &msg);
  void handleMedia(Message &msg);
  void handleValue(Message &msg);
  void sendACK(Message &incoming);
  void startResendTimer(Message *msg);
  void startRTTimer(Message *msg);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  int resendTimeoutMs;
  quint32 resendCounter;

  QSignalMapper *resendSignalMapper;
  QTime *rtTimers[MSG_TYPE_SUBTYPE_MAX];
  QTimer *resendTimers[MSG_TYPE_SUBTYPE_MAX];
  Message *resendMessages[MSG_TYPE_SUBTYPE_MAX];
  messageHandler messageHandlers[MSG_TYPE_SUBTYPE_MAX];

  QTimer *autoPing;
};

#endif

