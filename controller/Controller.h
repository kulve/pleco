/*
 * Copyright 2012-2020 Tuomas Kulve, <tuomas@kulve.fi>
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

#include <QApplication>

#include "Transmitter.h"
#include "VideoReceiver.h"
#include "AudioReceiver.h"
#include "Joystick.h"
#ifdef ENABLE_OPENHMD
#include "HMD.h"
#endif

class Controller : public QApplication
{
  Q_OBJECT;

 public:
  Controller(int &argc, char **argv);
  ~Controller(void);
  virtual void createGUI(void);
  virtual void connect(QString host, quint16 port);

  protected slots:
    void updateSpeedGracefully(void);
    void axisChanged(int axis, quint16 value);
    void updateCameraPeridiocally(void);
    void buttonChanged(int axis, quint16 value);

 protected:
    void sendCameraXYPending(void);
    void sendSpeedTurnPending(void);
    void sendCameraZoom(void);
    void sendCameraFocus(void);
    void sendVideoQuality(void);
    void sendCameraXY(void);
    void sendSpeedTurn(int speed, int turn);
    void sendVideoSource(int source);
    void enableLed(bool enable);
    void enableVideo(bool enable);
    void enableAudio(bool enable);

    Joystick *joystick;

    #ifdef ENABLE_OPENHMD
    HMD *hmd;
    #endif

    Transmitter *transmitter;
    VideoReceiver *vr;
    AudioReceiver *ar;

    int padCameraXPosition;
    int padCameraYPosition;

    double cameraX;
    double cameraY;

    int motorSpeedTarget;
    int motorSpeed;
    bool motorReverse;
    QTimer *motorSpeedUpdateTimer;
    int motorTurn;

    bool cameraXYPending;
    bool speedTurnPending;
    int speedTurnPendingSpeed;
    int speedTurnPendingTurn;

    int videoBufferPercent;
    int videoQuality;
    int cameraFocusPercent;
    int cameraZoomPercent;
    bool motorHalfSpeed;
    bool calibrate;
    bool audioState;
    bool videoState;
    bool ledState;

    QTimer *throttleTimerCameraXY;
    QTimer *throttleTimerSpeedTurn;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
