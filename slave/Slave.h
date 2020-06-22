/*
 * Copyright 2012 Tuomas Kulve, <tuomas@kulve.fi>
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
#include "AudioSender.h"
#include "ControlBoard.h"
#include "Camera.h"

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
    void sendSystemStats(void);
    void updateValue(quint8 type, quint16 value);
    void updateConnectionStatus(int status);
    void cbTemperature(quint16 value);
    void cbDistance(quint16 value);
    void cbCurrent(quint16 value);
    void cbVoltage(quint16 value);
    void sendCBPing(void);
    void turnOffRearLight(void);
    void speedTurnAckerman(quint8 speed, quint8 turn);
    void speedTurnTank(quint8 speed, quint8 turn);

 private:
    void parseSendVideo(quint16 value);
    void parseSendAudio(quint16 value);
    void parseCameraXY(quint16 value);
    void parseSpeedTurn(quint16 value);
    void parseVideoQuality(quint16 value);

    Transmitter *transmitter;
    VideoSender *vs;
    VideoSender *vs2;
    AudioSender *as;
    Hardware *hardware;
    ControlBoard *cb;
    Camera *camera;
    qint16 oldSpeed;
    qint16 oldTurn;
    quint8 oldDirectionLeft;
    quint8 oldDirectionRight;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
