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

 private slots:
  void readPendingDatagrams();
  void printError();

 private:
  void printData(QByteArray data);
  void parseData(QByteArray data);
  void msgSend(struct msg *msg);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  
};

#endif

