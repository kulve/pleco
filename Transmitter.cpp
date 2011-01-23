
#include <iostream>
#include "Transmitter.h"
#include "msg.h"


Transmitter::Transmitter(QString host, quint16 port):
  socket(), relayHost(host), relayPort(port)
{
  qDebug() << "in" << __FUNCTION__;
  // Nothing to do
}


Transmitter::~Transmitter()
{
  qDebug() << "in" << __FUNCTION__;
  // Nothing to do
}

void Transmitter::initSocket()
{
  qDebug() << "in " << __FUNCTION__;

  socket.bind(QHostAddress::Any, 0,QUdpSocket::ShareAddress);
  
  qDebug() << "Local address:" << socket.localAddress().toString();
  qDebug() << "Local port   :" << socket.localPort();

  connect(&socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
  connect(&socket, SIGNAL(error()), this, SLOT(printError()));
}



void Transmitter::ping()
{
  QByteArray datagram;
  datagram.append("p");
  
  qDebug() << "in" << __FUNCTION__;

  socket.writeDatagram(datagram, relayHost, relayPort);
}



void Transmitter::readPendingDatagrams()
{
  while (socket.hasPendingDatagrams()) {
	QByteArray datagram;
	QHostAddress sender;
	quint16 senderPort;

	qDebug() << "in" << __FUNCTION__;

	datagram.resize(socket.pendingDatagramSize());
	
	socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
	
	qDebug() << "Sender:" << sender.toString() << ", port:" << senderPort;
	printData(datagram);

	parseData(datagram);
  }
}


void Transmitter::printError()
{
  qDebug() << "Socket error: " << socket.errorString();
}



void Transmitter::printData(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__ << ", data:" << data.size();

  qDebug() << data.toHex();
}


void Transmitter::parseData(QByteArray data)
{
  struct msg *msg = msg_new();

  msg_data2msg(msg, (uint8_t*)data.data(), data.size());

  // TODO: validate CRC

  // Send ACK if the package is high priority
  if (msg_is_high_priority(msg)) {
	struct msg *ack = msg_new();
	
	ack->types |= MSG_TYPE_ACK;

	msgSend(msg);

	msg_free(ack);
  }

  msg_free(msg);
}
	

void Transmitter::msgSend(struct msg *msg)
{
  QByteArray datagram;

  qDebug() << "in" << __FUNCTION__;

  datagram.resize(msg_get_len(msg));

  msg_msg2data(msg, (uint8_t*)datagram.data(), datagram.size());
  
  socket.writeDatagram(datagram, relayHost, relayPort);
}


