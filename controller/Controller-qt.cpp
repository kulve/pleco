/*
 * Copyright 2020 Tuomas Kulve, <tuomas@kulve.fi>
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

#include "Controller-qt.h"

#if 0
#include <X11/Xlib.h>
#endif
Controller_qt::Controller_qt(int &argc, char **argv):
  Controller(argc, argv),
  window(NULL),
  textDebug(NULL),
  labelConnectionStatus(NULL),
  labelRTT(NULL),
  labelResendTimeout(NULL),
  labelUptime(NULL),
  labelVideoBufferPercent(NULL),
  labelLoadAvg(NULL),
  labelWlan(NULL),
  labelDistance(NULL),
  labelTemperature(NULL),
  labelCurrent(NULL),
  labelVoltage(NULL),
  horizSlider(NULL),
  vertSlider(NULL),
  buttonEnableCalibrate(NULL),
  buttonEnableVideo(NULL),
  buttonEnableAudio(NULL),
  buttonHalfSpeed(NULL),
  sliderVideoQuality(NULL),
  comboboxVideoSource(NULL),
  labelRx(NULL),
  labelTx(NULL),
  labelCalibrateSpeed(NULL),
  labelCalibrateTurn(NULL),
  labelSpeed(NULL),
  labelTurn(NULL),
  sliderZoom(NULL),
  sliderFocus(NULL)
{

}



Controller_qt::~Controller_qt(void)
{
  // Delete window
  if (window) {
    delete window;
    window = NULL;
  }
}



void Controller_qt::createGUI(void)
{
  Controller::createGUI();

  // If old top level window exists, delete it first
  if (window) {
    delete window;
  }

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

  // Update video jitter buffer fill percent once per second
  QTimer *videoBufferPercentTimer = NULL;
  videoBufferPercentTimer = new QTimer();
  videoBufferPercentTimer->setSingleShot(false);
  videoBufferPercentTimer->start(1000);
  QObject::connect(videoBufferPercentTimer, SIGNAL(timeout()), this, SLOT(updateVideoBufferPercent()));

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
  QObject::connect(buttonHalfSpeed, SIGNAL(clicked(bool)), this, SLOT(clickedHalfSpeed(void)));
  grid->addWidget(buttonHalfSpeed, row, 1);

  // Zoom slider (in %)
  sliderZoom = new QSlider(Qt::Horizontal);
  sliderZoom->setMinimum(0);
  sliderZoom->setMaximum(100);
  sliderZoom->setSliderPosition(0);

  label = new QLabel("Zoom:");
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderZoom, row, 1);
  QObject::connect(sliderZoom, SIGNAL(sliderMoved(int)), this, SLOT(cameraZoomChanged(void)));

  // Focus slider (in %)
  sliderFocus = new QSlider(Qt::Horizontal);
  sliderFocus->setMinimum(0);
  sliderFocus->setMaximum(100);
  sliderFocus->setSliderPosition(0);

  label = new QLabel("Focus:");
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderFocus, row, 1);
  QObject::connect(sliderFocus, SIGNAL(sliderMoved(int)), this, SLOT(cameraFocusChanged(void)));

  // VideoQuality slider
  label = new QLabel("Video quality:");
  sliderVideoQuality = new QSlider(Qt::Horizontal);
  sliderVideoQuality->setMinimum(0);
  sliderVideoQuality->setMaximum(4);
  sliderVideoQuality->setSliderPosition(0);
  grid->addWidget(label, ++row, 0);
  grid->addWidget(sliderVideoQuality, row, 1);
  QObject::connect(sliderVideoQuality, SIGNAL(sliderMoved(int)), this, SLOT(videoQualityChanged(void)));

  // Video jitter buffer status (lower the better)
  label = new QLabel("Video buffer (%):");
  labelVideoBufferPercent = new QLabel("");

  grid->addWidget(label, ++row, 0);
  grid->addWidget(labelVideoBufferPercent, row, 1);

  // Enable debug LED
  label = new QLabel("Led:");
  grid->addWidget(label, ++row, 0);
  buttonEnableLed = new QPushButton("LED");
  buttonEnableLed->setCheckable(true);
  QObject::connect(buttonEnableLed, SIGNAL(clicked(bool)), this, SLOT(clickedEnableLed(void)));
  grid->addWidget(buttonEnableLed, row, 1);

  // Enable video
  label = new QLabel("Video:");
  grid->addWidget(label, ++row, 0);
  buttonEnableVideo = new QPushButton("Enable");
  buttonEnableVideo->setCheckable(true);
  QObject::connect(buttonEnableVideo, SIGNAL(clicked(bool)), this, SLOT(clickedEnableVideo(void)));
  grid->addWidget(buttonEnableVideo, row, 1);

  // Video source
  label = new QLabel("Video source:");
  grid->addWidget(label, ++row, 0);
  comboboxVideoSource = new QComboBox();
  comboboxVideoSource->addItem("Camera");
  comboboxVideoSource->addItem("Test");
  grid->addWidget(comboboxVideoSource, row, 1);
  QObject::connect(comboboxVideoSource, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedVideoSource(int)));

  // Enable audio
  label = new QLabel("Audio:");
  grid->addWidget(label, ++row, 0);
  buttonEnableAudio = new QPushButton("Enable");
  buttonEnableAudio->setCheckable(true);
  QObject::connect(buttonEnableAudio, SIGNAL(clicked(bool)), this, SLOT(clickedEnableAudio(void)));
  grid->addWidget(buttonEnableAudio, row, 1);

  window->show();
}


#if 0
bool Controller_qt::x11EventFilter(XEvent *event)
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


void Controller_qt::connect(QString host, quint16 port)
{
  Controller::connect(host, port);

  QObject::connect(transmitter, SIGNAL(rtt(int)), this, SLOT(updateRtt(int)));
  QObject::connect(transmitter, SIGNAL(resendTimeout(int)), this, SLOT(updateResendTimeout(int)));
  QObject::connect(transmitter, SIGNAL(resentPackets(quint32)), this, SLOT(updateResentPackets(quint32)));
  QObject::connect(transmitter, SIGNAL(networkRate(int, int, int, int)), this, SLOT(updateNetworkRate(int, int, int, int)));
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));
  QObject::connect(transmitter, SIGNAL(periodicValue(quint8, quint16)), this, SLOT(updatePeriodicValue(quint8, quint16)));
  QObject::connect(transmitter, SIGNAL(debug(QString *)), this, SLOT(showDebug(QString *)));
  QObject::connect(transmitter, SIGNAL(connectionStatusChanged(int)), this, SLOT(updateConnectionStatus(int)));
}



void Controller_qt::updateRtt(int ms)
{
  qDebug() << "RTT:" << ms;
  if (labelRTT) {
    labelRTT->setText(QString::number(ms));
  }
}



void Controller_qt::updateResendTimeout(int ms)
{
  qDebug() << "ResendTimeout:" << ms;
  if (labelResendTimeout) {
    labelResendTimeout->setText(QString::number(ms));
  }
}



void Controller_qt::updateResentPackets(quint32 resendCounter)
{
  qDebug() << "ResentPackets:" << resendCounter;
  if (labelResentPackets) {
    labelResentPackets->setText(QString::number(resendCounter));
  }
}



void Controller_qt::updateCalibrateSpeed(int percent)
{
  qDebug() << "Calibrate Speed:" << percent;
  if (labelCalibrateSpeed) {
    labelCalibrateSpeed->setText(QString::number(percent));
  }
}



void Controller_qt::updateCalibrateTurn(int percent)
{
  qDebug() << "Calibrate Turn:" << percent;
  if (labelCalibrateTurn) {
    labelCalibrateTurn->setText(QString::number(percent));
  }
}



void Controller_qt::updateSpeed(int percent)
{
  qDebug() << "Speed:" << percent;
  if (labelSpeed) {
    labelSpeed->setText(QString::number(percent));
  }
}



void Controller_qt::updateTurn(int percent)
{
  qDebug() << "Turn:" << percent;
  if (labelTurn) {
    labelTurn->setText(QString::number(percent));
  }
}



void Controller_qt::selectedVideoSource(int index)
{
  Controller::sendVideoSource(index);
}



void Controller_qt::clickedEnableLed(void)
{
  Controller::enableLed(buttonEnableLed->isChecked());
}



void Controller_qt::clickedEnableVideo(void)
{
  Controller::enableVideo(buttonEnableVideo->isChecked());
}



void Controller_qt::clickedEnableAudio(void)
{
  Controller::enableAudio(buttonEnableAudio->isChecked());
}



void Controller_qt::clickedHalfSpeed(void)
{
  motorHalfSpeed = buttonHalfSpeed->isChecked();
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << motorHalfSpeed;

  // Rest is handled in updateMotor()
}



void Controller_qt::videoQualityChanged(void)
{
  videoQuality = sliderVideoQuality->sliderPosition();
  Controller::sendVideoQuality();
}



void Controller_qt::updateNetworkRate(int payloadRx, int totalRx, int payloadTx, int totalTx)
{
  if (labelRx) {
    labelRx->setText(QString::number(payloadRx) + " / " + QString::number(totalRx));
  }

  if (labelTx) {
    labelTx->setText(QString::number(payloadTx) + " / " + QString::number(totalTx));
  }
}



void Controller_qt::updateValue(quint8 type, quint16 value)
{
  qDebug() << "in" << __FUNCTION__ << ", type:" << type << ", value:" << value;

  switch (type) {
  default:
    qWarning("%s: Unhandled type: %d", __FUNCTION__, type);
  }
}



void Controller_qt::updatePeriodicValue(quint8 type, quint16 value)
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
    break;
  case MSG_SUBTYPE_UPTIME:
    if (labelUptime) {
      labelUptime->setNum(value);
    }
    break;
  default:
    qWarning("%s: Unhandled type: %d", __FUNCTION__, type);
  }
}



void Controller_qt::showDebug(QString *msg)
{
  qDebug() << "in" << __FUNCTION__ << ", debug msg:" << *msg;

  if (textDebug) {
    QTime t = QTime::currentTime();
    textDebug->append("<" + t.toString("hh:mm:ss") + "> " + *msg);
    textDebug->moveCursor(QTextCursor::End);
  }

  delete msg;
}


void Controller_qt::updateConnectionStatus(int connection)
{
  qDebug() << "in" << __FUNCTION__ << ", connection status:" << connection;

  if (labelConnectionStatus) {
    switch (connection) {
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



void Controller_qt::updateMotor(QKeyEvent *event)
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

  // Update GUI
  updateSpeed(speed);
  updateTurn(turn);

  // Send updated values
  sendSpeedTurn(speed, turn);
}


void Controller_qt::updateCamera(double x_percent, double y_percent)
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



void Controller_qt::updateCameraX(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  // reverse the direction
  cameraX = -1 * degree;

  sendCameraXY();
}



void Controller_qt::updateCameraY(int degree)
{
  qDebug() << "in" << __FUNCTION__ << ", degree:" << degree;

  // reverse the direction
  cameraY = -1 * degree;

  sendCameraXY();
}



void Controller_qt::cameraZoomChanged(void)
{
  cameraZoomPercent = sliderZoom->sliderPosition();
  sendCameraZoom();
}



void Controller_qt::cameraFocusChanged(void)
{
  cameraFocusPercent = sliderFocus->sliderPosition();
  sendCameraFocus();
}



/*
 * Timer calls this function peridiocally to update the video buffer fill percent
 */
void Controller_qt::updateVideoBufferPercent(void)
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
