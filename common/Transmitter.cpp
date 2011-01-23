
#include <iostream>
#include "Transmitter.h"


Transmitter::Transmitter(QString host, quint16 port):
  socket(), relayHost(host), relayPort(port), pingTimer(NULL)
{
  qDebug() << "in" << __FUNCTION__;
  // Nothing to do
}


Transmitter::~Transmitter()
{
  qDebug() << "in" << __FUNCTION__;

  // Remove sending ping timer, if any
  if (pingTimer) {
	pingTimer->stop();
	delete pingTimer;
	pingTimer = NULL;
  }

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
  qDebug() << "in" << __FUNCTION__;

  // Remove existing timer, if any
  if (pingTimer) {
	pingTimer->stop();
	delete pingTimer;
	pingTimer = NULL;
  }

  // Start a new ping every 500ms and ping once immediately
  pingTimer = new QTimer(this);
  connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));
  pingTimer->start(500);
  sendPing();
}



void Transmitter::sendPing()
{
  qDebug() << "in" << __FUNCTION__;

  QByteArray datagram;
  datagram.append("ping");

  socket.writeDatagram(datagram, relayHost, relayPort);
}



void Transmitter::pong()
{
  QByteArray datagram;
  datagram.append("pong");
  
  qDebug() << "in" << __FUNCTION__;

  // Send pong
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

  // Handle different sized messages in different methods
  switch (data.size()) {
  case 1: 
	parseData1(data);
	break;
  case 2:
	parseData2(data);
	break;
  case 3:
	parseData3(data);
	break;
  case 4:
	parseData4(data);
	break;
  default:
	qDebug() << "Unhandled message length:" << data.size();
  }
}



void Transmitter::parseData1(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__;

  // not implemented
}



void Transmitter::parseData2(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__;

  // not implemented
}



void Transmitter::parseData3(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__;

  // not implemented
}



void Transmitter::parseData4(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__;

  if (data.contains("ping")) {
	pong();
	return;
  }

  if (data.contains("pong")) {

	// Remove sending ping timer, if any
	if (pingTimer) {
	  pingTimer->stop();
	  delete pingTimer;
	  pingTimer = NULL;
	}

	return;
  }

  qDebug() << "Unhandled 4 byte message:" << data.toHex();
}
