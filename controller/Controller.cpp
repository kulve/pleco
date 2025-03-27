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

#include "Controller.h"
#include "Transmitter.h"
#include "VideoReceiver.h"
#include "AudioReceiver.h"
#include "Joystick.h"
#include "Message.h"

// Limit the frequency of sending commands affecting PWM signals
#define THROTTLE_FREQ_CAMERA_XY  50
#define THROTTLE_FREQ_SPEED_TURN 50

// Limit the motor speed change
#define MOTOR_SPEED_GRACE_LIMIT  10

Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv),
  joystick(nullptr),
  transmitter(nullptr),
  vr(nullptr),
  ar(nullptr),
  throttleTimerCameraXY(nullptr),
  throttleTimerSpeedTurn(nullptr)
{

}



Controller::~Controller(void)
{
  // Delete the joystick
  if (joystick != nullptr) {
    delete joystick;
    joystick = nullptr;
  }

  // Delete the transmitter
  if (transmitter != nullptr) {
    delete transmitter;
    transmitter = nullptr;
  }

  // Delete video receiver
  if (vr != nullptr) {
    delete vr;
    vr = nullptr;
  }

  // Delete audio receiver
  if (ar != nullptr) {
    delete ar;
    ar = nullptr;
  }
}



void Controller::createGUI(void)
{
  joystick = new Joystick();
  joystick->init();
  QObject::connect(joystick, SIGNAL(buttonChanged(int, quint16)), this, SLOT(buttonChanged(int, quint16)));
  QObject::connect(joystick, SIGNAL(axisChanged(int, quint16)), this, SLOT(axisChanged(int, quint16)));

  // Move camera peridiocally to the direction pointed by the joystick
  // FIXME: enable only if a gamepad is found?
  QTimer *joystickTimer = NULL;
  const int freq = 50;
  joystickTimer = new QTimer();
  joystickTimer->setSingleShot(false);
  joystickTimer->start(1000/freq);
  QObject::connect(joystickTimer, SIGNAL(timeout()), this, SLOT(updateCameraPeridiocally()));

  ar = new AudioReceiver();
}



void Controller::connect(QString host, quint16 port)
{

  // Delete old transmitter if any
  if (transmitter) {
    delete transmitter;
  }

  // Create a new transmitter
  transmitter = new Transmitter(host, port);

  transmitter->initSocket();

  QObject::connect(transmitter, SIGNAL(video(QByteArray *, quint8)), vr, SLOT(consumeVideo(QByteArray *, quint8)));
  QObject::connect(transmitter, SIGNAL(audio(QByteArray *)), ar, SLOT(consumeAudio(QByteArray *)));
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));
  QObject::connect(transmitter, SIGNAL(periodicValue(quint8, quint16)), this, SLOT(updatePeriodicValue(quint8, quint16)));

  QObject::connect(vr, SIGNAL(pos(double, double)), this, SLOT(updateCamera(double, double)));
  QObject::connect(vr, SIGNAL(motorControlEvent(QKeyEvent *)), this, SLOT(updateMotor(QKeyEvent *)));

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);

  // Get ready for receiving video
  vr->enableVideo(true);

  // Get ready for receiving audio
  ar->enableAudio(true);
}

void Controller::buttonChanged(int button, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", button:" << button << ", value:" << value;
  switch(button) {
  case JOYSTICK_BTN_REVERSE:
    motorReverse = (value == 1);
    break;
  default:
    qDebug() << "Joystick: Button unused:" << button;
  }
}



void Controller::enableLed(bool enable)
{
  if (ledState == enable) {
      return;
  }
  ledState = enable;

  qDebug() << "in" << __FUNCTION__ << ", enable:" << ledState;

  if (ledState) {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_LED, 1);
  } else {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_LED, 0);
  }

}



void Controller::enableVideo(bool enable)
{
  if (videoState == enable) {
    return;
  }
  videoState = enable;

  qDebug() << "in" << __FUNCTION__ << ", enable:" << videoState;

  if (videoState) {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 1);
  } else {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 0);
  }

}



void Controller::enableAudio(bool enable)
{
  if (audioState == enable) {
    return;
  }
  audioState = enable;

  qDebug() << "in" << __FUNCTION__ << ", enable:" << audioState;

  if (audioState) {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_AUDIO, 1);
  } else {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_AUDIO, 0);
  }
}

void Controller::sendVideoQuality(void)
{
  qDebug() << "in" << __FUNCTION__ << ", quality:" << videoQuality;
  transmitter->sendValue(MSG_SUBTYPE_VIDEO_QUALITY, videoQuality);
}



void Controller::sendVideoSource(int source)
{
  qDebug() << "in" << __FUNCTION__ << ", source:" << source;

  transmitter->sendValue(MSG_SUBTYPE_VIDEO_SOURCE, (quint16)source);
}



void Controller::sendCameraXYPending(void)
{
  if (cameraXYPending) {
    cameraXYPending = false;
    sendCameraXY();
  }
}



void Controller::sendCameraXY(void)
{
  // Send camera X and Y as percentages with 0.5 precision (i.e. doubled)
  quint8 x = static_cast<quint8>(((cameraX + 90) / 180.0) * 100 * 2);
  quint8 y = static_cast<quint8>(((cameraY + 90) / 180.0) * 100 * 2);

  quint16 value = (x << 8) | y;
  if (throttleTimerCameraXY == NULL) {
    throttleTimerCameraXY = new QTimer();
    throttleTimerCameraXY->setSingleShot(true);
    QObject::connect(throttleTimerCameraXY, SIGNAL(timeout()), this, SLOT(sendCameraXYPending()));
  }

  if (throttleTimerCameraXY->isActive()) {
    cameraXYPending = true;
    return;
  } else {
    throttleTimerCameraXY->start((int)(1000/(double)THROTTLE_FREQ_CAMERA_XY));
  }

  transmitter->sendValue(MSG_SUBTYPE_CAMERA_XY, value);
}



void Controller::sendSpeedTurnPending(void)
{
  if (speedTurnPending) {
    speedTurnPending = false;
    sendSpeedTurn(speedTurnPendingSpeed, speedTurnPendingTurn);
  }
}



void Controller::sendSpeedTurn(int speed, int turn)
{
  motorSpeed = speed;
  motorTurn = turn;

  if (throttleTimerSpeedTurn == NULL) {
    throttleTimerSpeedTurn = new QTimer();
    throttleTimerSpeedTurn->setSingleShot(true);
    QObject::connect(throttleTimerSpeedTurn, SIGNAL(timeout()), this, SLOT(sendSpeedTurnPending()));
  }

  if (throttleTimerSpeedTurn->isActive()) {
    speedTurnPending = true;
    speedTurnPendingSpeed = speed;
    speedTurnPendingTurn = turn;
    return;
  } else {
    throttleTimerSpeedTurn->start((int)(1000/(double)THROTTLE_FREQ_SPEED_TURN));
  }

  if (motorHalfSpeed) {
    speed /= 2;
  }

  // Send speed and turn as percentages shifted to 0 - 200
  quint8 x = static_cast<quint8>(speed + 100);
  quint8 y = static_cast<quint8>(turn + 100);
  quint16 value = (x << 8) | y;

  transmitter->sendValue(MSG_SUBTYPE_SPEED_TURN, value);
}



void Controller::sendCameraZoom(void)
{
  qDebug() << "in" << __FUNCTION__ << ", zoom:" << cameraZoomPercent;
  transmitter->sendValue(MSG_SUBTYPE_CAMERA_ZOOM, cameraZoomPercent);
}



void Controller::sendCameraFocus(void)
{
  qDebug() << "in" << __FUNCTION__ << ", focus:" << cameraFocusPercent;
  transmitter->sendValue(MSG_SUBTYPE_CAMERA_FOCUS, cameraFocusPercent);;
}



void Controller::updateSpeedGracefully(void)
{
  int change = abs(motorSpeed - motorSpeedTarget);

  // Limit the change of speed
  if (change > MOTOR_SPEED_GRACE_LIMIT) {
    change = MOTOR_SPEED_GRACE_LIMIT;

    if (motorSpeedUpdateTimer == NULL) {
      motorSpeedUpdateTimer = new QTimer();
      motorSpeedUpdateTimer->setSingleShot(true);
    }

    QObject::connect(motorSpeedUpdateTimer, SIGNAL(timeout()), this, SLOT(updateSpeedGracefully()));
    motorSpeedUpdateTimer->start(1000/THROTTLE_FREQ_SPEED_TURN);
  } else {
    // If no need to limit (we hit the target), make sure the timer is off
    if (motorSpeedUpdateTimer != NULL) {
      motorSpeedUpdateTimer->stop();
    }
  }

  if (motorSpeedTarget > motorSpeed) {
    motorSpeed += change;
  } else {
    motorSpeed -= change;
  }

  sendSpeedTurn(motorSpeed, motorTurn);
}



/*
 * This is called frequently to move the camera based on joystick position
 */
void Controller::updateCameraPeridiocally(void)
{
  // Experimented value
  const double scale = 0.03;
  bool sendUpdate = false;

  // X
  double movementX = padCameraXPosition * scale * -1;
  double oldValueX = cameraX;

  cameraX += movementX;
  if (cameraX < -90) {
    cameraX = -90;
  }
  if (cameraX > 90) {
    cameraX = 90;
  }

  if (qAbs(oldValueX - cameraX) > 0.5) {
    // FIXME: Update UI
    //horizSlider->setSliderPosition((int)(cameraX) * -1);
    sendUpdate = true;
  }

  // Y
  double movementY = padCameraYPosition * scale;
  double oldValueY = cameraY;

  cameraY -= movementY;
  if (cameraY < -90) {
    cameraY = -90;
  }
  if (cameraY > 90) {
    cameraY = 90;
  }

  if (qAbs(oldValueY - cameraY) > 0.5) {
    // FIXME: Update UI
    //vertSlider->setSliderPosition((int)(cameraY));
    sendUpdate = true;
  }

  if (sendUpdate) {
    sendCameraXY();
  }
}



void Controller::axisChanged(int axis, quint16 value)
{
  const int oversteer = 75;

  //qDebug() << "in" << __FUNCTION__ << ", axis:" << axis << ", value:" << value;

  switch(axis) {
  case JOYSTICK_AXIS_STEER:

    // Scale to +-100% with +-oversteering
    motorTurn = (int)((2 * 100 + 2 * oversteer) * (value/256.0) - (100 + oversteer));

    if (motorTurn > 100) {
      motorTurn = 100;
    } else if (motorTurn < -100) {
      motorTurn = -100;
    }

    // Update GUI
    // TODO: updateTurn(motorTurn);
    sendSpeedTurn(motorSpeed, motorTurn);
    break;
  case JOYSTICK_AXIS_THROTTLE:

    // Scale to +-100
    motorSpeedTarget = (int)(100 * (value/256.0));
    if (motorReverse) {
      motorSpeedTarget *= -1;
    }

    updateSpeedGracefully();
    break;
  case 2:
    padCameraXPosition = value - 128;
    break;
  case 3:
    padCameraYPosition = value - 128;
    break;
  default:
    qDebug() << "Joystick: Unhandled axis:" << axis;
    break;
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
