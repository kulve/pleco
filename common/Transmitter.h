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

#ifndef _TRANSMITTER_H
#define _TRANSMITTER_H

#include "Message.h"

#include <QtNetwork>
#include <QObject>

// Status messages for API user convenience. Not used in Transmitter.
// Bits of quint8
#define STATUS_VIDEO_ENABLED      0x1

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
  void sendMedia(QByteArray *media);
  void sendValue(quint8 type, quint16 value);

 private slots:
  void readPendingDatagrams();
  void printError(QAbstractSocket::SocketError error);
  void sendMessage(Message *msg);
  void sendMessage(QObject *msg);
  void updateRate(void);

 signals:
  void rtt(int ms);
  void resendTimeout(int ms);
  void resentPackets(quint32 resendCounter);
  void uptime(int seconds);
  void loadAvg(float avg);
  void wlan(int percent);
  void media(QByteArray *media);
  void value(quint8 type, quint16 value);
  void status(quint8 status);
  void networkRate(int payloadRx, int totalRx, int payloadTx, int totalTx);

 private:
  void printData(QByteArray *data);
  void parseData(QByteArray *data);
  void handleACK(Message &msg);
  void handlePing(Message &msg);
  void handleStats(Message &msg);
  void handleMedia(Message &msg);
  void handleValue(Message &msg);
  void sendACK(Message &incoming);
  void startResendTimer(Message *msg);
  void startRTTimer(Message *msg);

  QUdpSocket socket;
  QHostAddress relayHost;
  quint16 relayPort;
  int resendTimeoutMs;
  quint32 resendCounter;

  QSignalMapper *resendSignalMapper;
  QTime *rtTimers[MSG_TYPE_SUBTYPE_MAX];
  QTimer *resendTimers[MSG_TYPE_SUBTYPE_MAX];
  Message *resendMessages[MSG_TYPE_SUBTYPE_MAX];
  messageHandler messageHandlers[MSG_TYPE_SUBTYPE_MAX];

  QTimer *autoPing;

  // TX/RX rate
  int payloadSent;
  int payloadRecv;
  int totalSent;
  int totalRecv;
  QTimer rateTimer;
  QTime rateTime;
};

#endif

