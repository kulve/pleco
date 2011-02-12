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

 private slots:
  void updateRtt(int ms);
  void updateUptime(int seconds);
  void updateLoadAvg(float avg);
  void updateWlan(int percent);
  void updateCameraX(int degree);
  void updateCameraY(int degree);
  void sendCameraAndSpeed(void);

 private:
  void prepareSendCameraAndSpeed(void);

  Transmitter *transmitter;
  VideoReceiver *vr;

  QWidget *window;
  QLabel *labelUptime;
  QLabel *labelLoadAvg;
  QLabel *labelWlan;

  int cameraX;
  int cameraY;
  int motorRight;
  int motorLeft;

  QTimer *cameraAndSpeedTimer;
  QTime *cameraAndSpeedTime;
};

#endif
