/*
 * Copyright 2022 Tuomas Kulve, <tuomas@kulve.fi>
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

#ifndef _CONTROLLER_VR_H
#define _CONTROLLER_VR_H

#include "Controller.h"
#include "HMD.h"

class Controller_vr : public Controller
{
  Q_OBJECT;

 public:
  Controller_vr(int &argc, char **argv);
  ~Controller_vr(void);
  void createGUI(void);
  void connect(QString host, quint16 port);

  private slots:
    void updateRtt(int ms);
    void updateResendTimeout(int ms);
    void updateResentPackets(quint32 resendCounter);
    void updateCalibrateSpeed(int percent);
    void updateCalibrateTurn(int percent);
    void updateSpeed(int percent);
    void updateTurn(int percent);
    void clickedEnableLed(void);
    void clickedEnableVideo(void);
    void clickedEnableAudio(void);
    void clickedHalfSpeed(void);
    void selectedVideoSource(int index);
    void updateNetworkRate(int payloadRx, int totalRx, int payloadTx, int totalTx);
    void updateValue(quint8 type, quint16 value);
    void updatePeriodicValue(quint8 type, quint16 value);
    void showDebug(QString *msg);
    void updateConnectionStatus(int status);
    void updateMotor(QKeyEvent *event);
    void updateCamera(double x_percent, double y_percent);
    void updateCameraX(int degree);
    void updateCameraY(int degree);
    void cameraZoomChanged(void);
    void cameraFocusChanged(void);
    void videoQualityChanged(void);
    void updateVideoBufferPercent(void);

  private:
    HMD *hmd;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
