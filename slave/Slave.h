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

#ifndef _SLAVE_H
#define _SLAVE_H

#include "Transmitter.h"
#include "Hardware.h"
#include "VideoSender.h"

#include <QCoreApplication>
#include <QTimer>

class Slave : public QCoreApplication
{
  Q_OBJECT;

 public:
  Slave(int &argc, char **argv);
  ~Slave();
  bool init(void);
  void connect(QString host, quint16 port);

 private slots:
  void readStats(void);
  void updateValue(quint8 type, quint16 value);

 private:
  void sendStats(void);

  Transmitter *transmitter;
  QList<int> *stats;
  VideoSender *vs;
  quint8 status;
  Hardware *hardware;
};

#endif
