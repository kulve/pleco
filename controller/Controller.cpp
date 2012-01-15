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

#include <QApplication>
#include <QtGui>

#include "Controller.h"
#include "Transmitter.h"
#include "VideoReceiver.h"

#if 0
#include <X11/Xlib.h>
#endif
Controller::Controller(int &argc, char **argv):
  QApplication(argc, argv), 
  transmitter(NULL), vr(NULL), window(NULL),
  labelRTT(NULL), labelResendTimeout(NULL),
  labelUptime(NULL), labelLoadAvg(NULL), labelWlan(NULL),
  horizSlider(NULL), vertSlider(NULL),
  buttonEnableVideo(NULL), comboboxVideoSource(NULL),
  labelRx(NULL), labelTx(NULL)
{

}



Controller::~Controller(void)
{
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
  
  window = new QWidget();
  window->setWindowTitle("Controller");

  // Top level horizontal box
  QHBoxLayout *mainHoriz = new QHBoxLayout();
  window->setLayout(mainHoriz);
  window->resize(800, 600);
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
  mainHoriz->addLayout(grid);

  // Stats in separate horizontal boxes
  QLabel *label = NULL;

  int row = 0;

  // Round trip time
  label = new QLabel("RTT:");
  labelRTT = new QLabel("");

  grid->addWidget(label, row, 0, Qt::AlignLeft);
  grid->addWidget(labelRTT, row, 1, Qt::AlignLeft);

  // Resent Packets
  label = new QLabel("Resends:");
  labelResentPackets = new QLabel("0");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelResentPackets, row, 1, Qt::AlignLeft);

  // Resend timeout
  label = new QLabel("Resend timeout:");
  labelResendTimeout = new QLabel("");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelResendTimeout, row, 1, Qt::AlignLeft);

  // Uptime 
  label = new QLabel("Uptime:");
  labelUptime = new QLabel("");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelUptime, row, 1, Qt::AlignLeft);

  // Load Avg
  label = new QLabel("Load Avg:");
  labelLoadAvg = new QLabel("");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelLoadAvg, row, 1, Qt::AlignLeft);

  // Wlan
  label = new QLabel("Wlan:");
  labelWlan = new QLabel("");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelWlan, row, 1, Qt::AlignLeft);

  // Bytes received per second (payload / total)
  label = new QLabel("Payload/total Rx:");
  labelRx = new QLabel("0");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelRx, row, 1, Qt::AlignLeft);

  // Bytes sent per second (payload / total)
  label = new QLabel("Payload/total Tx:");
  labelTx = new QLabel("0");

  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  grid->addWidget(labelTx, row, 1, Qt::AlignLeft);

  // Enable video
  label = new QLabel("Video:");
  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  buttonEnableVideo = new QPushButton("Enable");
  buttonEnableVideo->setCheckable(true);
  QObject::connect(buttonEnableVideo, SIGNAL(clicked(bool)), this, SLOT(clickedEnableVideo(bool)));
  grid->addWidget(buttonEnableVideo, row, 1, Qt::AlignLeft);

  // Video source
  label = new QLabel("Video source:");
  grid->addWidget(label, ++row, 0, Qt::AlignLeft);
  comboboxVideoSource = new QComboBox();
  comboboxVideoSource->addItem("Test");
  comboboxVideoSource->addItem("Camera");
  grid->addWidget(comboboxVideoSource, row, 1, Qt::AlignLeft);
  QObject::connect(comboboxVideoSource, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedVideoSource(int)));

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
  QObject::connect(transmitter, SIGNAL(uptime(int)), this, SLOT(updateUptime(int)));
  QObject::connect(transmitter, SIGNAL(loadAvg(float)), this, SLOT(updateLoadAvg(float)));
  QObject::connect(transmitter, SIGNAL(wlan(int)), this, SLOT(updateWlan(int)));
  QObject::connect(transmitter, SIGNAL(media(QByteArray *)), vr, SLOT(consumeVideo(QByteArray *)));
  QObject::connect(transmitter, SIGNAL(status(quint8)), this, SLOT(updateStatus(quint8)));
  QObject::connect(transmitter, SIGNAL(networkRate(int, int, int, int)), this, SLOT(updateNetworkRate(int, int, int, int)));
  QObject::connect(transmitter, SIGNAL(value(quint8, quint16)), this, SLOT(updateValue(quint8, quint16)));

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



void Controller::updateUptime(int seconds)
{
  qDebug() << "uptime:" << seconds << "seconds";
  if (labelUptime) {
	labelUptime->setText(QString::number(seconds));
  }
}



void Controller::updateLoadAvg(float avg)
{
  qDebug() << "Load avg:" << avg;
  if (labelLoadAvg) {
	labelLoadAvg->setText(QString::number(avg));
  }
}



void Controller::updateWlan(int percent)
{
  qDebug() << "WLAN:" << percent;
  if (labelWlan) {
	labelWlan->setText(QString::number(percent));
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



void Controller::clickedEnableVideo(bool enabled)
{
  qDebug() << "in" << __FUNCTION__ << ", enabled:" << enabled << ", checked:" << buttonEnableVideo->isChecked();

  if (buttonEnableVideo->isChecked()) {
	transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 1);
  } else {
	transmitter->sendValue(MSG_SUBTYPE_ENABLE_VIDEO, 0);
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

