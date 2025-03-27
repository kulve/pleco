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

/*
 * Partly copied from:
 * TODO
  */

#include "Controller-vr.h"

Controller_vr::Controller_vr(int &argc, char **argv):
  Controller(argc, argv),
  hmd(nullptr),
  window(nullptr),
{

}



Controller_vr::~Controller_vr(void)
{
  if (hmd != nullptr) {
    delete hmd;
    hmd = nullptr;
  }
}



void Controller_vr::createGUI(void)
{
  Controller::createGUI();

  //hmd = new HMD();
  //hmd->init();

  VRWindow window;
  window.init();
  QSurfaceFormat fmt;
  fmt.setDepthBufferSize(24);
  fmt.setStencilBufferSize(8);
  window.setFormat(fmt);
  window.showMaximized();

  vr = new VideoReceiver(window);

  vr->setFixedSize(800, 600);
}



void Controller_vr::connect(QString host, quint16 port)
{
  Controller::connect(host, port);
}





void Controller_vr::updateMotor(QKeyEvent *event)
{
  int speed;
  int turn;

  // Key release events concerns only A and D
  if (event->isAutoRepeat() ||
      (event->type() == QEvent::KeyRelease &&
       (event->key() != Qt::Key_A && event->key() != Qt::Key_D))) {
    return;
  }

  turn = motorTurn;
  speed  = motorSpeed;

  switch(event->key()) {
  case Qt::Key_0:
    speed = 0;
    turn = 0;
    break;
  case Qt::Key_W:
    speed += 25;
    if (speed > 100) speed = 100;
    break;
  case Qt::Key_S:
    speed -= 25;
    if (speed < -100) speed = -100;
    break;
  case Qt::Key_A:
    if (1) {
      // Same as speed, 25% per press
      turn -= 25;
      if (turn < -100) turn = -100;
    } else {
      // Rock crawler on/off
      if (event->type() == QEvent::KeyPress) {
        turn = -100;
      } else if (event->type() == QEvent::KeyRelease) {
        turn = 0;
      }
    }
    break;
  case Qt::Key_D:
    if (1) {
      // Same as speed, 25% per press
      turn += 25;
      if (turn > 100) turn = 100;
    } else {
      // Rock crawler on/off
      if (event->type() == QEvent::KeyPress) {
        turn = 100;
      } else if (event->type() == QEvent::KeyRelease) {
        turn = 0;
      }
    }
    break;
  case Qt::Key_X:
    speed = 0;
    break;
  default:
    qWarning("Unhandled key: %d", event->key());
  }

  // Do nothing if no change
  if (speed == motorSpeed && turn == motorTurn) {
    return;
  }

  qDebug() << "in" << __FUNCTION__ << ", motorSpeed:" << motorSpeed << ", motorTurn:" << motorTurn;

  // Send updated values
  sendSpeedTurn(speed, turn);
}


/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
