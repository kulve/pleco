#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <QtNetwork>
#include <QObject>

class Transmitter : public QObject
{
  Q_OBJECT;
	
 public:
  Transmitter(QString host, quint16 port);
  ~Transmitter();
  void initSocket();

 public slots:
  void ping();
  void pong();

 private slots:
  void readPendingDatagrams();
  void printError();
  void sendPing();

 private:
  void printData(QByteArray data);
  void parseData(QByteArray data);
  void parseData1(QByteArray data);
  void parseData2(QByteArray data);
  void parseData3(QByteArray data);
  void parseData4(QByteArray data);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  QTimer *pingTimer;
};

#endif

