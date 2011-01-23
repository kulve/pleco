
#include "Transmitter.h"
#include "Message.h"


Transmitter::Transmitter(QString host, quint16 port):
  socket(), relayHost(host), relayPort(port), pingTimer(NULL), rttimer(NULL)
{
  qDebug() << "in" << __FUNCTION__;

  // Set message handlers
  for (int i = 0; i < 256; i++)  {
	messageHandlers[i] = NULL;
  }

  messageHandlers[Message::MSG_TYPE_PING] = &Transmitter::handlePing;
  messageHandlers[Message::MSG_TYPE_PONG] = &Transmitter::handlePong;
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

  connect(&socket, SIGNAL(readyRead()),
		  this, SLOT(readPendingDatagrams()));
  connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), 
		  this, SLOT(printError(QAbstractSocket::SocketError)));
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

  Message msg(Message::MSG_TYPE_PING);

  socket.writeDatagram(msg.data(), relayHost, relayPort);

  // Start round trip timer or restarting old one if existing
  if (rttimer) {
	rttimer->restart();
  } else {
	rttimer = new QTime();
	rttimer->start();
  }
}



void Transmitter::pong()
{
  qDebug() << "in" << __FUNCTION__;

  Message msg(Message::MSG_TYPE_PONG);

  // Send pong
  socket.writeDatagram(msg.data(), relayHost, relayPort);
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


void Transmitter::printError(QAbstractSocket::SocketError error)
{
  qDebug() << "Socket error (" << error << "):" << socket.errorString();
}



void Transmitter::printData(QByteArray data)
{
  qDebug() << "in" << __FUNCTION__ << ", data:" << data.size();

  qDebug() << data.toHex();
}


void Transmitter::parseData(QByteArray data)
{
  Message msg(data);

  if (!msg.isValid()) {
	return;
  }

  // Handle different message types in different methods
  if (messageHandlers[msg.type()]) {
	messageHandler func = messageHandlers[msg.type()];
	(this->*func)(msg);
  } else {
	qDebug() << "No message handler for type" << msg.type() << ", ignoring";
  }  
}



void Transmitter::handlePing(Message &)
{
  qDebug() << "in" << __FUNCTION__;


  // Reply to ping with pong. Always. Doesn't matter if it's a resend or not.
  pong();
}



void Transmitter::handlePong(Message &)
{
  qDebug() << "in" << __FUNCTION__;
  
  // Get round trip time for sending the ping to receiving the pong
  if (rttimer) {
	emit rtt(rttimer->elapsed());
	delete rttimer;
	rttimer = NULL;
  }
  
  // Remove sending ping timer, if any
  if (pingTimer) {
	pingTimer->stop();
	delete pingTimer;
	pingTimer = NULL;
  }
}
