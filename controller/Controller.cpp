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

#include <QApplication>
#include <QtGui>
#include <QVBoxLayout>

#include "Controller.h"
#include "Transmitter.h"
#include "VideoReceiver.h"
#include "Joystick.h"
#include "Message.h"

// Limit the frequency of sending commands affecting PWM signals
#define THROTTLE_FREQ_CAMERA_XY  50
#define THROTTLE_FREQ_SPEED_TURN 50

// Limit the motor speed change
#define MOTOR_SPEED_GRACE_LIMIT  10

#if 0
#include <X11/Xlib.h>
#endif
Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv),
  joystick(NULL),
  transmitter(NULL), vr(NULL), window(NULL), textDebug(NULL),
  labelConnectionStatus(NULL), labelRTT(NULL), labelResendTimeout(NULL),
  labelUptime(NULL), labelVideoBufferPercent(NULL), labelLoadAvg(NULL), labelWlan(NULL),
  labelDistance(NULL), labelTemperature(NULL),
  labelCurrent(NULL), labelVoltage(NULL),
  horizSlider(NULL), vertSlider(NULL), buttonEnableCalibrate(NULL),
  buttonEnableVideo(NULL), buttonHalfSpeed(NULL), sliderVideoQuality(NULL), comboboxVideoSource(NULL),
  labelRx(NULL), labelTx(NULL), 
  labelCalibrateSpeed(NULL), labelCalibrateTurn(NULL),
  labelSpeed(NULL), labelTurn(NULL), sliderZoom(NULL), sliderFocus(NULL),
  padCameraXPosition(0), padCameraYPosition(0),
  cameraX(0), cameraY(0),
  motorSpeedTarget(0), motorSpeed(0), motorSpeedUpdateTimer(NULL), motorTurn(0),
  calibrateSpeed(0), calibrateTurn(0),
  throttleTimerCameraXY(NULL), throttleTimerSpeedTurn(NULL),
  cameraXYPending(false), speedTurnPending(false),
  speedTurnPendingSpeed(0), speedTurnPendingTurn(0)
{

}



Controller::~Controller(void)
{
  // Delete the joystick
  if (joystick) {
    delete joystick;
    joystick = NULL;
  }

  // Delete the transmitter
  if (transmitter) {
    delete transmitter;
    transmitter = NULL;
  }

  // Delete video receiver
  if (vr) {
    delete vr;
    vr = NULL;
  }

  // Delete debug textEdit
  if (textDebug) {
    delete textDebug;
    textDebug = NULL;
  }

  // Delete window
  if (window) {
    delete window;
    window = NULL;
  }
}



void Controller::createGUI(void)
{
  // If old top level window exists, delete it first
  if (window) {
    delete window;
  }

  QSettings settings("Pleco", "Controller");
  calibrateSpeed = settings.value("calibrate/speed", 0).toInt();
  calibrateTurn = settings.value("calibrate/turn", 0).toInt();

  window = new QWidget();
  window->setWindowTitle("Controller");

  // Top level vertical box (debug view + the rest)
  QVBoxLayout *mainVert = new QVBoxLayout();  

  window->setLayout(mainVert);
  window->resize(1280, 720);

  // Horizontal box for "the rest"
  QHBoxLayout *mainHoriz = new QHBoxLayout();
  mainVert->addLayout(mainHoriz);

  // TextEdit for the debug prints
  textDebug = new QTextEdit("Debug prints from the slave");
  textDebug->setReadOnly(true);
  mainVert->addWidget(textDebug);

  // Vertical box with slider and the camera screen
  QVBoxLayout *screenVert = new QVBoxLayout();
  mainHoriz->addLayout(screenVert);
  
  horizSlider = new QSlider(Qt::Horizontal);
  horizSlider->setMinimum(-90);
  horizSlider->setMaximum(+90);
  horizSlider->setSliderPosition(0);
  QObject::connect(horizSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraX(int)));
  screenVert->addWidget(horizSlider);

  vr = new VideoReceiver(window);
  vr->setFixedSize(800, 600);
  screenVert->addWidget(vr);
  // Vertical slider next to camera screen
  vertSlider = new QSlider(Qt::Vertical);
  vertSlider->setMinimum(-90);
  vertSlider->setMaximum(+90);
  vertSlider->setSliderPosition(0);
  mainHoriz->addWidget(vertSlider);
  QObject::connect(vertSlider, SIGNAL(sliderMoved(int)), this, SLOT(updateCameraY(int)));

  // Grid layoyt for the slave stats
  QGridLayout *grid = new QGridLayout();
  grid->setColumnStretch(0, 4);
  grid->setColumnStretch(1, 6);
  mainHoriz->addLayout(grid, 10);

  // Stats in separate horizontal boxes
  QLabel *label = NULL;

  int row = 0;

  // Connection status
  label = new QLabel("Connection:");
  labelConnectionStatus = new QLabel("Lost");

  grid->addWidget(label, row, 0);
  grid->addWidget(labelConnectionStatus, row, 1);

  // Round trip time
  label = new QLabel("RTT:");
  labelRTT = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelRTT, row, 1);

  // Resent Packets
  label = new QLabel("Resends:");
  labelResentPackets = new QLabel("0");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelResentPackets, row, 1);

  // Resend timeout
  label = new QLabel("Resend timeout:");
  labelResendTimeout = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelResendTimeout, row, 1);

  // Uptime
  label = new QLabel("Uptime:");
  labelUptime = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelUptime, row, 1);

  // Load Avg
  label = new QLabel("Load Avg:");
  labelLoadAvg = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelLoadAvg, row, 1);

  // Wlan
  label = new QLabel("Wlan signal (%):");
  labelWlan = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelWlan, row, 1);

  // Distance
  label = new QLabel("Distance (m):");
  labelDistance = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelDistance, row, 1);

  // Temperature
  label = new QLabel("Temperature (C):");
  labelTemperature = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelTemperature, row, 1);

  // Current
  label = new QLabel("Current (A):");
  labelCurrent = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelCurrent, row, 1);

  // Voltage
  label = new QLabel("Voltage (V):");
  labelVoltage = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelVoltage, row, 1);

  // Bytes received per second (payload / total)
  label = new QLabel("Payload/total Rx:");
  labelRx = new QLabel("0");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelRx, row, 1);

  // Bytes sent per second (payload / total)
  label = new QLabel("Payload/total Tx:");
  labelTx = new QLabel("0");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelTx, row, 1);

  // Speed in %
  label = new QLabel("Cal. Speed:");
  labelCalibrateSpeed = new QLabel(QString::number(calibrateSpeed));

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelCalibrateSpeed, row, 1);

  // Turn in %
  label = new QLabel("Cal. Turn:");
  labelCalibrateTurn = new QLabel(QString::number(calibrateTurn));

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelCalibrateTurn, row, 1);

  // Speed in %
  label = new QLabel("Speed:");
  labelSpeed = new QLabel("0");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelSpeed, row, 1);

  // Turn in %
  label = new QLabel("Turn:");
  labelTurn = new QLabel("0");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelTurn, row, 1);

  // Half speed
  label = new QLabel("Half speed:");
  grid->addWidget(label, ++row, 0);
  buttonHalfSpeed = new QPushButton("Half speed");
  buttonHalfSpeed->setCheckable(true);
  buttonHalfSpeed->setChecked(true);
  QObject::connect(buttonHalfSpeed, SIGNAL(clicked(bool)), this, SLOT(clickedHalfSpeed(bool)));
  grid->addWidget(buttonHalfSpeed, row, 1);

  // Zoom slider (in %)
  sliderZoom = new QSlider(Qt::Horizontal);
  sliderZoom->setMinimum(0);
  sliderZoom->setMaximum(100);
  sliderZoom->setSliderPosition(0);

  label = new QLabel("Zoom:");
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderZoom, row, 1);
  QObject::connect(sliderZoom, SIGNAL(sliderMoved(int)), this, SLOT(sendCameraZoom(void)));

  // Focus slider (in %)
  sliderFocus = new QSlider(Qt::Horizontal);
  sliderFocus->setMinimum(0);
  sliderFocus->setMaximum(100);
  sliderFocus->setSliderPosition(0);

  label = new QLabel("Focus:");
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderFocus, row, 1);
  QObject::connect(sliderFocus, SIGNAL(sliderMoved(int)), this, SLOT(sendCameraFocus(void)));

  // VideoQuality slider
  label = new QLabel("Video quality:");
  sliderVideoQuality = new QSlider(Qt::Horizontal);
  sliderVideoQuality->setMinimum(0);
  sliderVideoQuality->setMaximum(2);
  sliderVideoQuality->setSliderPosition(0);
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderVideoQuality, row, 1);
  QObject::connect(sliderVideoQuality, SIGNAL(sliderMoved(int)), this, SLOT(sendVideoQuality(void)));

  // Video jitter buffer status (lower the better)
  label = new QLabel("Video buffer (%):");
  labelVideoBufferPercent = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelVideoBufferPercent, row, 1);

  // Enable calibrate
  label = new QLabel("Calibrate:");
  grid->addWidget(label, ++row, 0);
  buttonEnableCalibrate = new QPushButton("Calibrate");
  buttonEnableCalibrate->setCheckable(true);
  QObject::connect(buttonEnableCalibrate, SIGNAL(clicked(bool)), this, SLOT(clickedEnableCalibrate(bool)));
  grid->addWidget(buttonEnableCalibrate, row, 1);

  // Enable debug LED
  label = new QLabel("Led:");
  grid->addWidget(label, ++row, 0);
  buttonEnableLed = new QPushButton("LED");
  buttonEnableLed->setCheckable(true);
  QObject::connect(buttonEnableLed, SIGNAL(clicked(bool)), this, SLOT(clickedEnableLed(bool)));
  grid->addWidget(buttonEnableLed, row, 1);

  // Enable video
  label = new QLabel("Video:");
  grid->addWidget(label, ++row, 0);
  buttonEnableVideo = new QPushButton("Enable");
  buttonEnableVideo->setCheckable(true);
  QObject::connect(buttonEnableVideo, SIGNAL(clicked(bool)), this, SLOT(clickedEnableVideo(bool)));
  grid->addWidget(buttonEnableVideo, row, 1);

  // Video source
  label = new QLabel("Video source:");
  grid->addWidget(label, ++row, 0);
  comboboxVideoSource = new QComboBox();
  comboboxVideoSource->addItem("Camera");
  comboboxVideoSource->addItem("Test");
  grid->addWidget(comboboxVideoSource, row, 1);
  QObject::connect(comboboxVideoSource, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedVideoSource(int)));

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

  // Update video jitter buffer fill percent once per second
  QTimer *videoBufferPercentTimer = NULL;
  videoBufferPercentTimer = new QTimer();
  videoBufferPercentTimer->setSingleShot(false);
  videoBufferPercentTimer->start(1000);
  QObject::connect(videoBufferPercentTimer, SIGNAL(timeout()), this, SLOT(updateVideoBufferPercent()));

  window->show();
}


#if 0
bool Controller::x11EventFilter(XEvent *event)
{
  qDebug() << "X11 event";
  XButtonEvent *e  = (XButtonEvent*)event;
  XMotionEvent *em = (XMotionEvent*)event;

  switch (event->type) {
  case ButtonPress:
    qDebug() <<"Caught ButtonPress XEvent";
    qDebug() << "Button, x, y:" << e->button << e->x << e->y;
    break;
  case ButtonRelease:
    qDebug() <<"Caught ButtonRelease XEvent";
    qDebug() << "Button, x, y:" << e->button << e->x << e->y;
    break;
  case MotionNotify:
    qDebug() <<"Caught MotionNotify XEvent";
    qDebug() << "x, y:" << em->x_root << em->y_root;
    break;
  case EnterNotify:
    qDebug() <<"Caught EnterNotify XEvent";
    break;
  case LeaveNotify:
    qDebug() <<"Caught LeaveNotify XEvent";
    break;
  case FocusIn:
    qDebug() <<"Caught FocusIn XEvent";
    break;
  case FocusOut: 
    qDebug() <<"Caught FocusOut XEvent";
    break;
  case KeymapNotify:
    qDebug() <<"Caught KeymapNotify XEvent";
    break;
  case Expose:
    qDebug() <<"Caught Expose XEvent";
  case GraphicsExpose:
    qDebug() <<"Caught GraphicsExpose XEvent";
    break;
  case NoExpose:
    qDebug() <<"Caught NoExpose XEvent";
    break;
  case CirculateRequest:
    qDebug() <<"Caught CirculateRequest XEvent";
    break;
  case ConfigureRequest:
    qDebug() <<"Caught ConfigureRequest XEvent";
    break;
  case MapRequest:
    qDebug() <<"Caught MapRequest XEvent";
    break;
  case ResizeRequest:
    qDebug() <<"Caught ResizeRequest XEvent";
    break;
  case CirculateNotify:
    qDebug() <<"Caught CirculateNotify XEvent";
    break;
  case ConfigureNotify:
    qDebug() <<"Caught ConfigureNotify XEvent";
    break;
  case CreateNotify:
    qDebug() <<"Caught CreateNotify XEvent";
    break;
  case DestroyNotify:
    qDebug() <<"Caught DestroyNotify XEvent";
    break;
  case GravityNotify:
    qDebug() <<"Caught GravityNotify XEvent";
    break;
  case MapNotify:
    qDebug() <<"Caught MapNotify XEvent";
    break;
  case MappingNotify:
    qDebug() <<"Caught MappingNotify XEvent";
    break;
  case ReparentNotify:
    qDebug() <<"Caught ReparentNotify XEvent";
    break;
  case UnmapNotify:
    qDebug() <<"Caught UnmapNotify XEvent";
    break;
  case VisibilityNotify:
    qDebug() <<"Caught VisibilityNotify XEvent";
    break;
  case ColormapNotify:
    qDebug() <<"Caught ColormapNotify XEvent";
    break;
  case ClientMessage:
    qDebug() <<"Caught ClientMessage XEvent";
    break;
  case PropertyNotify:
    qDebug() <<"Caught PropertyNotify XEvent";
    break;
  case SelectionClear:
    qDebug() <<"Caught SelectionClear XEvent";
    break;
  case SelectionNotify:
    qDebug() <<"Caught SelectionNotify XEvent";
    break;
  case SelectionRequest:
    qDebug() <<"Caught SelectionRequest XEvent";
    break;
  default:
    qDebug() <<"Caught event: " << event->type;
  }
  return false;
}
#endif


void Controller::connect(QString host, quint16 port)
{

  // Delete old transmitter if any
  if (transmitter) {
    delete transmitter;
  }

  // Create a new transmitter
  transmitter = new Transmitter(host, port);

  transmitter->initSocket();

  QObject::connect(transmitter, SIGNAL(rtt(int)), this, SLOT(updateRtt(int)));
  QObject::connect(transmitter, SIGNAL(resendTimeout(int)), this, SLOT(updateResendTimeout(int)));
  QObject::connect(transmitter, SIGNAL(resentPackets(quint32)), this, SLOT(updateResentPackets(quint32)));
  QObject::connect(transmitter, SIGNAL(media(QByteArray *)), vr, SLOT(consumeVideo(QByteArray *)));
  QObject::connect(transmitter, SIGNAL(status(quint8)), this, SLOT(updateStatus(quint8)));
  QObject::connect(transmitter, SIGNAL(networkRate(int, int, int, int)), this, SLOT(updateNetworkRate(int, int, int, int)));
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));
  QObject::connect(transmitter, SIGNAL(periodicValue(quint8, quint16)), this, SLOT(updatePeriodicValue(quint8, quint16)));
  QObject::connect(transmitter, SIGNAL(debug(QString *)), this, SLOT(showDebug(QString *)));
  QObject::connect(transmitter, SIGNAL(connectionStatusChanged(int)), this, SLOT(updateConnectionStatus(int)));

  QObject::connect(vr, SIGNAL(pos(double, double)), this, SLOT(updateCamera(double, double)));
  QObject::connect(vr, SIGNAL(motorControlEvent(QKeyEvent *)), this, SLOT(updateMotor(QKeyEvent *)));

  // Send ping every second (unless other high priority packet are sent)
  transmitter->enableAutoPing(true);

  // Get ready for receiving video
  vr->enableVideo(true);
}



void Controller::updateRtt(int ms)
{
  qDebug() << "RTT:" << ms;
  if (labelRTT) {
    labelRTT->setText(QString::number(ms));
  }
}



void Controller::updateResendTimeout(int ms)
{
  qDebug() << "ResendTimeout:" << ms;
  if (labelResendTimeout) {
    labelResendTimeout->setText(QString::number(ms));
  }
}



void Controller::updateResentPackets(quint32 resendCounter)
{
  qDebug() << "ResentPackets:" << resendCounter;
  if (labelResentPackets) {
    labelResentPackets->setText(QString::number(resendCounter));
  }
}



void Controller::updateStatus(quint8 status)
{
  qDebug() << "in" << __FUNCTION__ << ", status:" << status;

  // Check if the video sending is active
  if (status & STATUS_VIDEO_ENABLED) {
    buttonEnableVideo->setChecked(true);
    buttonEnableVideo->setText("Disable");
  } else {
    buttonEnableVideo->setChecked(false);
    buttonEnableVideo->setText("Enable");
  }

}



void Controller::updateCalibrateSpeed(int percent)
{
  qDebug() << "Calibrate Speed:" << percent;
  if (labelCalibrateSpeed) {
    labelCalibrateSpeed->setText(QString::number(percent));
  }
}



void Controller::updateCalibrateTurn(int percent)
{
  qDebug() << "Calibrate Turn:" << percent;
  if (labelCalibrateTurn) {
    labelCalibrateTurn->setText(QString::number(percent));
  }
}



void Controller::updateSpeed(int percent)
{
  qDebug() << "Speed:" << percent;
  if (labelSpeed) {
    labelSpeed->setText(QString::number(percent));
  }
}



void Controller::updateTurn(int percent)
{
  qDebug() << "Turn:" << percent;
  if (labelTurn) {
    labelTurn->setText(QString::number(percent));
  }
}



void Controller::clickedEnableCalibrate(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonEnableCalibrate->isChecked();

  // Everything is handled in updateMotor()
}



void Controller::clickedEnableLed(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonEnableLed->isChecked();

  if (buttonEnableLed->isChecked()) {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_LED, 1);
  } else {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_LED, 0);
  }

}



void Controller::clickedEnableVideo(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonEnableVideo->isChecked();

  if (buttonEnableVideo->isChecked()) {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 1);
  } else {
    transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 0);
  }

}



void Controller::clickedHalfSpeed(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonHalfSpeed->isChecked();

  // Everything is handled in updateMotor()
}



void Controller::sendVideoQuality(void)
{
  if (sliderVideoQuality) {
    qDebug() << "in" << __FUNCTION__ << ", quality:" << sliderVideoQuality->sliderPosition();
    transmitter->sendValue(MSG_SUBTYPE_VIDEO_QUALITY, sliderVideoQuality->sliderPosition());
  }
}



void Controller::selectedVideoSource(int index)
{
  qDebug() << "in" << __FUNCTION__ << ", index:" << index;

  transmitter->sendValue(MSG_SUBTYPE_VIDEO_SOURCE, (quint16)index);
}



void Controller::updateNetworkRate(int payloadRx, int totalRx, int payloadTx, int totalTx)
{
  if (labelRx) {
    labelRx->setText(QString::number(payloadRx) + " / " + QString::number(totalRx));
  }

  if (labelTx) {
    labelTx->setText(QString::number(payloadTx) + " / " + QString::number(totalTx));
  }
}



void Controller::updateValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << type << ", value:" << value;

  switch (type) {
  default:
    qWarning("%s: Unhandled type: %d", __FUNCTION__, type);
  }
}



void Controller::updatePeriodicValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << Message::getSubTypeStr(type) << ", value:" << value;

  switch (type) {
  case MSG_SUBTYPE_DISTANCE:
    if (labelDistance) {
      labelDistance->setNum(value/100.0);
    }
    break;
  case MSG_SUBTYPE_TEMPERATURE:
    if (labelTemperature) {
      labelTemperature->setNum(value/100.0);
    }
    break;
  case MSG_SUBTYPE_BATTERY_CURRENT:
    if (labelCurrent) {
      labelCurrent->setNum(value/1000.0);
    }
    break;
  case MSG_SUBTYPE_BATTERY_VOLTAGE:
    if (labelVoltage) {
      labelVoltage->setNum(value/1000.0);
    }
    break;
  case MSG_SUBTYPE_CPU_USAGE:
    if (labelLoadAvg) {
      labelLoadAvg->setNum(value/100.0);
    }
    break;
  case MSG_SUBTYPE_SIGNAL_STRENGTH:
    if (labelWlan) {
      labelWlan->setNum(value);
    }
  case MSG_SUBTYPE_UPTIME:
    if (labelUptime) {
      labelUptime->setNum(value);
    }
    break;
    break;
  default:
    qWarning("%s: Unhandled type: %d", __FUNCTION__, type);
  }
}



void Controller::showDebug(QString *msg)
{
  qDebug() << "in" << __FUNCTION__ << ", debug msg:" << *msg;

  if (textDebug) {
    QTime t = QTime::currentTime();
    textDebug->append("<" + t.toString("hh:mm:ss") + "> " + *msg);
    textDebug->moveCursor(QTextCursor::End);
  }

  delete msg;
}


void Controller::updateConnectionStatus(int status)
{
  qDebug() << "in" << __FUNCTION__ << ", status:" << status;

  if (labelConnectionStatus) {
    switch (status) {
    case CONNECTION_STATUS_OK:
      labelConnectionStatus->setText("OK");
      break;
    case CONNECTION_STATUS_RETRYING:
      labelConnectionStatus->setText("RETRYING");
      break;
    case CONNECTION_STATUS_LOST:
      labelConnectionStatus->setText("LOST");
      break;
    }
  }
}



void Controller::updateMotor(QKeyEvent *event)
{

  int oldSpeed;
  int speed;
  int oldTurn;
  int turn;

  // Key release events concerns only A and D
  if (event->isAutoRepeat() ||
      (event->type() == QEvent::KeyRelease &&
       (event->key() != Qt::Key_A && event->key() != Qt::Key_D))) {
    return;
  }

  // If we are calibrating, adjust the calibration values instead of
  // the real ones
  if (buttonEnableCalibrate->isChecked()) {
    turn = calibrateTurn;
    speed  = calibrateSpeed;
  } else {
    turn = motorTurn;
    speed  = motorSpeed;
  }

  oldTurn = turn;
  oldSpeed = speed;

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
    if (event->type() == QEvent::KeyPress) {
      turn = -100;
    } else if (event->type() == QEvent::KeyRelease) {
      turn = 0;
    }
    break;
  case Qt::Key_D:
    if (event->type() == QEvent::KeyPress) {
      turn = 100;
    } else if (event->type() == QEvent::KeyRelease) {
      turn = 0;
    }
    break;
  case Qt::Key_X:
    speed = 0;
    break;
  default:
    qWarning("Unhandled key: %d", event->key());
  }

  // Do nothing if no change
  if (oldSpeed == speed && oldTurn == turn) {
    return;
  }

  if (buttonEnableCalibrate->isChecked()) {
    if (speed != calibrateSpeed || turn != calibrateTurn) {
      calibrateSpeed = speed;
      calibrateTurn = turn;

      qDebug() << "in" << __FUNCTION__ << ", calibrateSpeed:" << calibrateSpeed << ", calibrateTurn:" << calibrateTurn;

      // Update GUI
      updateCalibrateSpeed(calibrateSpeed);
      updateCalibrateTurn(calibrateTurn);

      // Send only calibration values for tuning zero offset
      sendSpeedTurn(calibrateSpeed, calibrateTurn);

      // Permanently store new calibration values
      QSettings settings("Pleco", "Controller");
      settings.setValue("calibrate/speed", calibrateSpeed);
      settings.setValue("calibrate/turn", calibrateTurn);
    }
  } else {
    if (speed != motorSpeed || turn != motorTurn) {
      motorSpeed = speed;
      motorTurn = turn;

      qDebug() << "in" << __FUNCTION__ << ", motorSpeed:" << motorSpeed << ", motorTurn:" << motorTurn;

      // Update GUI
      updateSpeed(motorSpeed);
      updateTurn(motorTurn);

      // Calculate speed after calibrated offset of zero
      int s = motorSpeed;
      int t = motorTurn;

      if (calibrateSpeed) {
        if (motorSpeed > 0) {
          s = static_cast<int>((100 - calibrateSpeed) * (motorSpeed / 100.0));
        } else {
          s = static_cast<int>((100 + calibrateSpeed) * (motorSpeed / 100.0));
        }
      }

      if (calibrateTurn) {
        if (motorTurn > 0) {
          t = static_cast<int>((100 - calibrateTurn) * (motorTurn / 100.0));
        } else {
          t = static_cast<int>((100 + calibrateTurn) * (motorTurn / 100.0));
        }
      }

      // Send calibrated values
      sendSpeedTurn(calibrateSpeed + s, calibrateTurn + t);
    }
  }
}


void Controller::updateCamera(double x_percent, double y_percent)
{

  // Convert percents to degrees (+-90) and reverse
  cameraX = 180 * x_percent;
  cameraY = 180 * y_percent;

  cameraX = 180 - cameraX;
  //cameraY = 180 - cameraY;

  cameraX -= 90;
  cameraY -= 90;

  //qDebug() << "in" << __FUNCTION__ << ", degrees (X Y):" << x_degree << y_degree;

  // revert the slider positions
  horizSlider->setSliderPosition((int)(cameraX * -1));
  vertSlider->setSliderPosition((int)(cameraY));

  sendCameraXY();
}



void Controller::updateCameraX(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  // reverse the direction
  cameraX = -1 * degree;

  sendCameraXY();
}



void Controller::updateCameraY(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  // reverse the direction
  cameraY = -1 * degree;

  sendCameraXY();
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

  if (buttonHalfSpeed->isChecked()) {
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
  if (sliderZoom) {
    qDebug() << "in" << __FUNCTION__ << ", zoom:" << sliderZoom->sliderPosition();
    transmitter->sendValue(MSG_SUBTYPE_CAMERA_ZOOM, sliderZoom->sliderPosition());
  }
}



void Controller::sendCameraFocus(void)
{
  if (sliderFocus) {
    qDebug() << "in" << __FUNCTION__ << ", focus:" << sliderFocus->sliderPosition();
    transmitter->sendValue(MSG_SUBTYPE_CAMERA_FOCUS, sliderFocus->sliderPosition());
  }
}



void Controller::buttonChanged(int button, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", button:" << button << ", value:" << value;
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

  // Update GUI
  updateSpeed(motorSpeed);

  sendSpeedTurn(motorSpeed, motorTurn);
}



void Controller::axisChanged(int axis, quint16 value)
{
  const int oversteer = 75;

  //qDebug() << "in" << __FUNCTION__ << ", axis:" << axis << ", value:" << value;

  switch(axis) {
  case 0:

    // Scale to +-100% with +-oversteering
    motorTurn = (int)((2 * 100 + 2 * oversteer) * (value/256.0) - (100 + oversteer));

    if (motorTurn > 100) {
      motorTurn = 100;
    } else if (motorTurn < -100) {
      motorTurn = -100;
    }

    // Update GUI
    updateTurn(motorTurn);
    sendSpeedTurn(motorSpeed, motorTurn);
    break;
  case 1:
    // Scale to +-100 and reverse
    motorSpeedTarget = (int)(200 * (value/256.0) - 100);
    motorSpeedTarget *= -1;

    updateSpeedGracefully();
    break;
  case 2:
    padCameraXPosition = value - 128;
    break;
  case 3:
    padCameraYPosition = value - 128;
    break;
  }
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
    horizSlider->setSliderPosition((int)(cameraX) * -1);
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
    vertSlider->setSliderPosition((int)(cameraY));
    sendUpdate = true;
  }

  if (sendUpdate) {
    sendCameraXY();
  }
}



/*
 * Timer calls this function peridiocally to update the video buffer fill percent
 */
void Controller::updateVideoBufferPercent(void)
{
  if (labelVideoBufferPercent) {
    labelVideoBufferPercent->setNum(vr->getBufferFilled());
  }
}

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
