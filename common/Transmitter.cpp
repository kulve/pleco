/*
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Transmitter.h"
#include "Message.h"

#define RESEND_TIMEOUT 1000


Transmitter::Transmitter(QString host, quint16 port):
  socket(), relayHost(host), relayPort(port), resendTimeoutMs(RESEND_TIMEOUT),
  resendCounter(0), autoPing(NULL),
  payloadSent(0), payloadRecv(0), totalSent(0), totalRecv(0), rateTimer(), rateTime()
{
  qDebug() << "in" << __FUNCTION__ << ", connecting to host:" << host << ", port:" << port;

  // Add a signal mapper for resending packages
  resendSignalMapper = new QSignalMapper(this);
  connect(resendSignalMapper, SIGNAL(mapped(QObject *)),
		  this, SLOT(sendMessage(QObject *)));

  // Zero arrays
  // FIXME: memset?
  for (int i = 0; i < MSG_TYPE_SUBTYPE_MAX; i++)  {
	rtTimers[i] = NULL;
	resendTimers[i] = NULL;
	messageHandlers[i] = NULL;
	resendMessages[i] = NULL;
  }

  // Set message handlers
  messageHandlers[MSG_TYPE_ACK]                = &Transmitter::handleACK;
  messageHandlers[MSG_TYPE_PING]               = &Transmitter::handlePing;
  messageHandlers[MSG_TYPE_STATS]              = &Transmitter::handleStats;
  messageHandlers[MSG_TYPE_MEDIA]              = &Transmitter::handleMedia;
  messageHandlers[MSG_TYPE_VALUE]              = &Transmitter::handleValue;
}



Transmitter::~Transmitter()
{
  qDebug() << "in" << __FUNCTION__;

  delete resendSignalMapper;

  // Delete the timers 
  for (int i = 0; i < MSG_TYPE_SUBTYPE_MAX; i++)  {
	delete rtTimers[i];
	rtTimers[i] = NULL;

	delete resendTimers[i];
	resendTimers[i] = NULL;

	messageHandlers[i] = NULL;
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


  // Start RX/TX timers
  // FIXME: stop these somewhere?
  rateTimer.start(1000);
  rateTime.start();
  connect(&rateTimer, SIGNAL(timeout()), 
		  this, SLOT(updateRate()));

}



void Transmitter::enableAutoPing(bool enable)
{

  if (!enable) {
	// Disable autopinging, if running

	if (autoPing) {
	  autoPing->stop();
	  delete autoPing;
	  autoPing = NULL;
	  return;
	}
  }
  
  // Start autopinging, if not not running already
  if (autoPing) {
	return;
  }

  autoPing = new QTimer();

  // Send ping every second (sending a high priority package restarts the timer)
  connect(autoPing, SIGNAL(timeout()), this, SLOT(sendPing()));
  autoPing->start(1000);
}



void Transmitter::sendPing()
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_PING);
  sendMessage(msg);
}



void Transmitter::sendStats(QList <int> *stats)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_STATS);

  QByteArray *data = msg->data();

  for (int i = 0; i < stats->size(); i++) {
	(*data)[TYPE_OFFSET_PAYLOAD + i] = (quint8)stats->at(i);
  }

  sendMessage(msg);
}



void Transmitter::sendMedia(QByteArray *media)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_MEDIA);

  // Append media payload
  msg->data()->append(*media);
  
  // FIXME: illogical to delete in sendMedia but not in other send* methods?
  delete media;

  sendMessage(msg);
}



void Transmitter::sendValue(quint8 subType, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << subType << ", value:" << value;

  Message *msg = new Message(MSG_TYPE_VALUE, subType);

  msg->setPayload16(value);

  sendMessage(msg);
}



void Transmitter::sendMessage(Message *msg)
{
  msg->setCRC();

  printData(msg->data());

  int tx = socket.writeDatagram(*msg->data(), relayHost, relayPort);
  if (tx == -1) {
	qWarning() << "Failed to writeDatagram:" << socket.errorString();
  } else {
	payloadSent += tx;
	totalSent += tx + 28; // UDP + IPv4 headers.
  }


  // If not a high priority package, all is done
  if (!msg->isHighPriority()) {
	delete msg;
	return;
  }

  // Reset auto ping timer (unless sending a ping)
  if (autoPing && msg->type() != MSG_TYPE_PING) {
	autoPing->start();
  }

  // Store pointer to message until it's acked
  // FIXME: are we leaking here?
  resendMessages[msg->fullType()] = msg;

  // Start high priority package resend
  startResendTimer(msg);

  // Start (or restart) round trip timer
  startRTTimer(msg);
}



void Transmitter::sendMessage(QObject *msg)
{
  sendMessage(dynamic_cast<Message *>(msg));
}



void Transmitter::startResendTimer(Message *msg)
{
  quint16 fullType = msg->fullType();

  // Restart existing timer, or create a new one
  if (resendTimers[fullType]) {
	resendTimers[fullType]->start();
	emit(resentPackets(++resendCounter));
  } else {
	resendTimers[fullType] = new QTimer(this);

	// Connect to resend timeout through a signal mapper
	connect(resendTimers[fullType], SIGNAL(timeout()), resendSignalMapper, SLOT(map()));
  }

  // If there is existing mapping, free it before setting a new one
  QObject *existing = resendSignalMapper->mapping(resendTimers[fullType]);
  if (existing) {
	qDebug() << "deleting existing";
	delete existing;
  }

  resendSignalMapper->setMapping(resendTimers[fullType], msg);

  resendTimers[fullType]->start(resendTimeoutMs);
}



void Transmitter::startRTTimer(Message *msg)
{
  quint16 fullType = msg->fullType();

  // Restart existing stopwatch, or create a new one
  if (rtTimers[fullType]) {
	rtTimers[fullType]->restart();
  } else {
	rtTimers[fullType] = new QTime();
	rtTimers[fullType]->start();
  }
}



void Transmitter::readPendingDatagrams()
{
  while (socket.hasPendingDatagrams()) {
	QByteArray datagram;
	QHostAddress sender;
	quint16 senderPort;

	qDebug() << "in" << __FUNCTION__;

	datagram.resize(socket.pendingDatagramSize());
	
	int rx = socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
	if (rx == -1) {
	  qWarning() << "Failed to readDatagram:" << socket.errorString();
	} 

	payloadRecv += rx;
	totalRecv += rx + 28; // UDP + IPv4 headers

	qDebug() << "Sender:" << sender.toString() << ", port:" << senderPort;
	printData(&datagram);

	parseData(&datagram);
  }
}


void Transmitter::printError(QAbstractSocket::SocketError error)
{
  qDebug() << "Socket error (" << error << "):" << socket.errorString();
}



void Transmitter::printData(QByteArray *data)
{
  qDebug() << "in" << __FUNCTION__ << ", data len:" << data->size();

  if (data->size() > 32) {
	qDebug() << "Big packet (video?), not printing content";
  } else {
	qDebug() << data->toHex();
  }
}



void Transmitter::parseData(QByteArray *data)
{
  qDebug() << "in" << __FUNCTION__;

  Message msg(*data);

  // isValid() also checks that the packet is exactly as long as expected
  if (!msg.isValid()) {
	qWarning() << "Package not valid, ignoring";
	return;
  }

  qDebug() << __FUNCTION__ << ": type:" << (int)msg.type();

  // Check, whether to ACK the packet
  if (msg.isHighPriority()) {
	sendACK(msg);
  }

  // Handle different message types in different methods
  if (messageHandlers[msg.type()]) {
	messageHandler func = messageHandlers[msg.type()];
	(this->*func)(msg);
  } else {
	qWarning() << "No message handler for type" << msg.type() << ", ignoring";
  }  
}



void Transmitter::sendACK(Message &incoming)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_ACK);

  // Sets CRC as well
  msg->setACK(incoming);

  sendMessage(msg);
}



void Transmitter::handleACK(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  quint16 ackedFullType = msg.getAckedFullType();

  // FIXME: validate acked CRC

  // Stop and delete resend timer
  if (resendTimers[ackedFullType]) {
	resendTimers[ackedFullType]->stop();
	resendSignalMapper->removeMappings(resendTimers[ackedFullType]);
	delete resendTimers[ackedFullType];
	resendTimers[ackedFullType] = NULL;
  } else {
	  qWarning() << "No Resend timer running for type" << ackedFullType;
  }


  // Send RTT signal and delete RT timer
  if (rtTimers[ackedFullType]) {
	int rttMs = rtTimers[ackedFullType]->elapsed();

	emit(rtt(rttMs));

	// Adjust resend timeout but keep it always > 20ms.
	// If the doubled round trip time is less than current timeout, decrease resendTimeoutMs by 10%.
	// if the doubled round trip time is greater that current resendTimeoutMs, increase resendTimeoutMs to 2x rtt
	if (2 * rttMs < resendTimeoutMs) {
	  resendTimeoutMs -= (int)(0.1 * resendTimeoutMs);
	} else {
	  resendTimeoutMs = 2 * rttMs;
	}

	if (resendTimeoutMs < 20) {
	  resendTimeoutMs = 20;
	}

	emit(resendTimeout(resendTimeoutMs));

	qDebug() << "New resend timeout:" << resendTimeoutMs;

	delete rtTimers[ackedFullType];
	rtTimers[ackedFullType] = NULL;
  } else {
	  qWarning() << "No RT timer running for type" << ackedFullType;
  }

  // Delete the message waiting for resend
  if (resendMessages[ackedFullType]) {
	delete resendMessages[ackedFullType];
	resendMessages[ackedFullType] = NULL;
  }
}



void Transmitter::handlePing(Message &)
{
  qDebug() << "in" << __FUNCTION__;

  // We don't do anything with ping (ACKing it is enough).
  // This handler is here to avoid missing handler warning.
}



void Transmitter::handleStats(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  QList <int> stats;
  
  // FIXME: no hardcoded limit
  for (int i = 0; i < 6; i++) {
	stats.append((int)((quint8)msg.data()->at(TYPE_OFFSET_PAYLOAD + i)));
  }

  // FIXME: no hardcoded indexes
  int index = 0;
  emit(status(stats[index++]));              // Status as on/off bits
  emit(uptime(stats[index++] * 60));         // Uptime is sent as minutes, but
							                 // seconds is normal representation
  emit(loadAvg(float(stats[index++]) / 10)); // Load avg is sent 10x
  emit(wlan(stats[index++]));                // WLAN signal, 0-100%
}



void Transmitter::handleMedia(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  QByteArray *data = new QByteArray(*msg.data());

  // Remove header from the data to get the actual media payload
  data->remove(0, TYPE_OFFSET_PAYLOAD);

  // Send the received media payload to the application
  emit(media(data));
}



void Transmitter::handleValue(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  quint8 type = msg.subType();

  char *data = msg.data()->data();

  quint16 *p = (quint16 *)&data[TYPE_OFFSET_PAYLOAD];

  quint16 val = *p;

  // Emit the enable/disable signal
  emit(value(type, val));
}



void Transmitter::updateRate(void)
{

  // Time in ms since last update
  int elapsedMs = rateTime.restart();

  // Bytes received per second on average since last update
  int payloadRx = (int)(payloadRecv / (elapsedMs/(double)1000));
  payloadRecv = 0;
  int totalRx = (int)(totalRecv / (elapsedMs/(double)1000));
  totalRecv = 0;

  // Bytes sent per second on average since last update
  int payloadTx = (int)(payloadSent / (elapsedMs/(double)1000));
  payloadSent = 0;
  int totalTx = (int)(totalSent / (elapsedMs/(double)1000));
  totalSent = 0;

  emit(networkRate(payloadRx, totalRx, payloadTx, totalTx));
}
