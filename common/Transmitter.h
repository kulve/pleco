#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include "Message.h"

#include <QtNetwork>
#include <QObject>

class Transmitter : public QObject
{
  Q_OBJECT;

 public:

  // Message handler function prototype
  typedef void (Transmitter::*messageHandler)(Message &msg);

  Transmitter(QString host, quint16 port);
  ~Transmitter();
  void initSocket();

 public slots:
  void ping();
  void pong();

 private slots:
  void readPendingDatagrams();
  void printError(QAbstractSocket::SocketError error);
  void sendPing();

 signals:
  void rtt(int ms);

 private:
  void printData(QByteArray data);
  void parseData(QByteArray data);
  void handlePing(Message &msg);
  void handlePong(Message &msg);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  QTimer *pingTimer;
  QTime *rttimer;
  messageHandler messageHandlers[256];
};

#endif

