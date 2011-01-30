#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "Message.h"

#include <QtNetwork>
#include <QObject>

#define RESEND_TIMEOUT 1000

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

 private slots:
  void readPendingDatagrams();
  void printError(QAbstractSocket::SocketError error);
  void sendMessage(Message *msg);
  void sendMessage(QObject *msg);

 signals:
  void rtt(int ms);
  void uptime(int seconds);
  void loadAvg(float avg);
  void wlan(int percent);
  void cameraX(int degrees);
  void cameraY(int degrees);
  void motorRight(int percent);
  void motorLeft(int percent);

 private:
  void printData(QByteArray data);
  void parseData(QByteArray data);
  void handleACK(Message &msg);
  void handlePing(Message &msg);
  void handleStats(Message &msg);
  void handleCameraAndSpeed(Message &msg);
  void sendACK(Message &incoming);
  void startResendTimer(Message *msg);
  void startRTTimer(Message *msg);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  int resendTimeout;

  QSignalMapper *resendSignalMapper;
  QTime *rtTimers[Message::MSG_TYPE_MAX];
  QTimer *resendTimers[Message::MSG_TYPE_MAX];
  Message *resendMessages[Message::MSG_TYPE_MAX];
  messageHandler messageHandlers[Message::MSG_TYPE_MAX];

  QTimer *autoPing;
};

#endif

