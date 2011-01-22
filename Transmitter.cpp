#include "Transmitter.h"

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
  }
}


void Transmitter::printError()
{
  qDebug() << "Socket error: " << socket.errorString();
}
