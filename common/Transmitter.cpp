/*
 * Copyright 2015 Tuomas Kulve, <tuomas@kulve.fi>
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

#define RESEND_TIMEOUT_DEFAULT 1000


Transmitter::Transmitter(QString host, quint16 port):
  socket(), relayHost(host), relayPort(port), resendTimeoutMs(RESEND_TIMEOUT_DEFAULT),
  resendCounter(0), connectionTimeoutTimer(NULL), connectionStatus(CONNECTION_STATUS_LOST), 
  autoPing(NULL),
  payloadSent(0), payloadRecv(0), totalSent(0), totalRecv(0), rateTimer(), rateTime()
{
  qDebug() << "in" << __FUNCTION__ << ", connecting to host:" << host << ", port:" << port;

  // Add a signal mapper for resending packages
  resendSignalMapper = new QSignalMapper(this);
  connect(resendSignalMapper, SIGNAL(mapped(QObject *)),
          this, SLOT(resendMessage(QObject *)));

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
  messageHandlers[MSG_TYPE_VIDEO]              = &Transmitter::handleVideo;
  messageHandlers[MSG_TYPE_AUDIO]              = &Transmitter::handleAudio;
  messageHandlers[MSG_TYPE_DEBUG]              = &Transmitter::handleDebug;
  messageHandlers[MSG_TYPE_VALUE]              = &Transmitter::handleValue;
  messageHandlers[MSG_TYPE_PERIODIC_VALUE]     = &Transmitter::handlePeriodicValue;
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
    }
    return;
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



void Transmitter::sendVideo(QByteArray *video, quint8 index)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_VIDEO, index);

  // Append media payload
  msg->data()->append(*video);

  // FIXME: illogical to delete in sendVideo but not in other send* methods?
  delete video;

  sendMessage(msg);
}



void Transmitter::sendAudio(QByteArray *audio)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_AUDIO);

  // Append media payload
  msg->data()->append(*audio);
  
  // FIXME: illogical to delete in sendAudio but not in other send* methods?
  delete audio;

  sendMessage(msg);
}



void Transmitter::sendDebug(QString *debug)
{
  qDebug() << "in" << __FUNCTION__;

  Message *msg = new Message(MSG_TYPE_DEBUG);

  debug->truncate(MSG_DEBUG_MAX_LEN);

  // Append debug payload
  msg->data()->append(*debug);
  
  // FIXME: illogical to delete in sendMedia but not in other send* methods?
  delete debug;

  sendMessage(msg);
}



void Transmitter::sendValue(quint8 subType, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << Message::getSubTypeStr(subType) << ", value:" << value;

  Message *msg = new Message(MSG_TYPE_VALUE, subType);

  msg->setPayload16(value);

  sendMessage(msg);
}



void Transmitter::sendPeriodicValue(quint8 subType, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << Message::getSubTypeStr(subType) << ", value:" << value;

  Message *msg = new Message(MSG_TYPE_PERIODIC_VALUE, subType);

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

  // Reset auto ping timer if sending High Prio (or ack) packet (unless sending a ping)
  if (autoPing && (msg->isHighPriority() || msg->type() == MSG_TYPE_ACK) && msg->type() != MSG_TYPE_PING) {
    autoPing->start();
  }

  // If not a high priority package, all is done
  if (!msg->isHighPriority()) {
    delete msg;
    return;
  }

  // Start connection timeout timer
  startConnectionTimeout();

  // Store pointer to message until it's acked
  if (resendMessages[msg->fullType()]) {
    delete resendMessages[msg->fullType()];
  }
  resendMessages[msg->fullType()] = msg;

  // Start high priority package resend
  startResendTimer(msg);

  // Start (or restart) round trip timer
  startRTTimer(msg);
}



void Transmitter::resendMessage(QObject *msgobj)
{
  Message *msg = dynamic_cast<Message *>(msgobj);

  Q_ASSERT(resendMessages[msg->fullType()] != NULL);

  emit(resentPackets(++resendCounter));

  if (connectionStatus == CONNECTION_STATUS_OK) {
    connectionStatus = CONNECTION_STATUS_RETRYING;
    emit(connectionStatusChanged(connectionStatus));
  }
  resendMessages[msg->fullType()] = NULL;

  sendMessage(msg);
}



void Transmitter::startResendTimer(Message *msg)
{
  quint16 fullType = msg->fullType();

  // Restart existing timer, or create a new one
  if (!resendTimers[fullType]) {
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



void Transmitter::startConnectionTimeout(void)
{
  // Start existing timer (if inactive), or create a new one
  if (!connectionTimeoutTimer) {
    connectionTimeoutTimer = new QTimer(this);
    connectionTimeoutTimer->setSingleShot(true);
    connect(connectionTimeoutTimer, SIGNAL(timeout()), this, SLOT(connectionTimeout()));
  }

  if (!connectionTimeoutTimer->isActive()) {
    // FIXME: 4 * resendTimeoutMs but never less than e.g. 2 secs?
    connectionTimeoutTimer->start(4 * resendTimeoutMs);
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

  qDebug() << __FUNCTION__ << ": type:" << Message::getTypeStr((int)msg.type());

  // New data -> connection ok
  if (connectionStatus != CONNECTION_STATUS_OK) {
    connectionStatus = CONNECTION_STATUS_OK;
    emit(connectionStatusChanged(connectionStatus));
  }

  // Stop connection timeout timer as we got something from the slave
  if (connectionTimeoutTimer && connectionTimeoutTimer->isActive()) {
    connectionTimeoutTimer->stop();
  }

  // Check, whether to ACK the packet
  if (msg.isHighPriority()) {
    sendACK(msg);
  }

  // Handle different message types in different methods
  if (messageHandlers[msg.type()]) {
    messageHandler func = messageHandlers[msg.type()];
    (this->*func)(msg);
  } else {
    qWarning() << "No message handler for type" << Message::getTypeStr(msg.type()) << ", ignoring";
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
  quint16 ackedCRC = msg.getAckedCRC();

  // If the ack is not for the latest msg, ignore it
  if (resendMessages[ackedFullType] &&
      !resendMessages[ackedFullType]->matchCRC(ackedCRC)) {
    // We got ack, just not for the latest package. Restart timer to avoid continuous resends.
    if (resendTimers[ackedFullType]) {
      resendTimers[ackedFullType]->start();
    }
    qDebug() << __FUNCTION__ << ": acked CRC does not match for type:" << ackedFullType;
    return;
  }

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



void Transmitter::handleVideo(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  QByteArray *data = new QByteArray(*msg.data());

  // Remove header from the data to get the actual video payload
  data->remove(0, TYPE_OFFSET_PAYLOAD);

  // Send the received video payload to the application
  emit(video(data, msg.subType()));
}



void Transmitter::handleAudio(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  QByteArray *data = new QByteArray(*msg.data());

  // Remove header from the data to get the actual audio payload
  data->remove(0, TYPE_OFFSET_PAYLOAD);

  // Send the received audio payload to the application
  emit(audio(data));
}



void Transmitter::handleDebug(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  // Remove header from the data to get the actual debug payload
  QByteArray data(*msg.data());
  data.remove(0, TYPE_OFFSET_PAYLOAD);

  QString *debugmsg = new QString(data.data());

  // Send the received debug message to the application
  emit(debug(debugmsg));
}



void Transmitter::handleValue(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  quint8 type = msg.subType();
  quint16 val = msg.getPayload16();

  // Emit the enable/disable signal
  emit(value(type, val));
}



void Transmitter::handlePeriodicValue(Message &msg)
{
  qDebug() << "in" << __FUNCTION__;

  quint8 type = msg.subType();
  quint16 val = msg.getPayload16();

  emit(periodicValue(type, val));
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



void Transmitter::connectionTimeout(void)
{
  qDebug() << "in" << __FUNCTION__;

  // FIXME: stop all resends
  resendTimeoutMs = RESEND_TIMEOUT_DEFAULT;

  if (connectionStatus != CONNECTION_STATUS_LOST) {
    connectionStatus = CONNECTION_STATUS_LOST;
    emit(connectionStatusChanged(connectionStatus));
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
