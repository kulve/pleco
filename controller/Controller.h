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

#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "Transmitter.h"
#include "VideoReceiver.h"

#include <QApplication>
#include <QtGui>

class Controller : public QApplication
{
  Q_OBJECT;

 public:
  Controller(int &argc, char **argv);
  ~Controller(void);
  void createGUI(void);
  void connect(QString host, quint16 port);
#if 0
  bool x11EventFilter(XEvent *event);
#endif

 private slots:
  void updateRtt(int ms);
  void updateResendTimeout(int ms);
  void updateResentPackets(quint32 resendCounter);
  void updateUptime(int seconds);
  void updateLoadAvg(float avg);
  void updateWlan(int percent);
  void updateStatus(quint8 status);
  void clickedEnableVideo(bool enabled);
  void selectedVideoSource(int index);
  void updateNetworkRate(int payloadRx, int totalRx, int payloadTx, int totalTx);
  void updateValue(quint8 type, quint16 value);

 private:
  Transmitter *transmitter;
  VideoReceiver *vr;

  QWidget *window;
  QLabel *labelRTT;
  QLabel *labelResendTimeout;
  QLabel *labelResentPackets;
  QLabel *labelUptime;
  QLabel *labelLoadAvg;
  QLabel *labelWlan;

  QSlider *horizSlider;
  QSlider *vertSlider;

  QPushButton *buttonEnableVideo;
  QComboBox *comboboxVideoSource;

  QLabel *labelRx;
  QLabel *labelTx;

};

#endif
