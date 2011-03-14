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
  void updateResendTimeout(int ms);
  void updateUptime(int seconds);
  void updateLoadAvg(float avg);
  void updateWlan(int percent);
  void updateCamera(double x_percent, double y_percent);
  void updateCameraX(int degree);
  void updateCameraY(int degree);
  void updateMotor(QKeyEvent *event);
  void updateMotorRight(int percent);
  void updateMotorLeft(int percent);
  void updateStatus(quint8 status);
  void sendCameraAndSpeed(void);
  void clickedEnableVideo(bool enabled);
  void selectedVideoSource(int index);

 private:
  void prepareSendCameraAndSpeed(void);

  Transmitter *transmitter;
  VideoReceiver *vr;

  QWidget *window;
  QLabel *labelRTT;
  QLabel *labelResendTimeout;
  QLabel *labelUptime;
  QLabel *labelLoadAvg;
  QLabel *labelWlan;

  QSlider *horizSlider;
  QSlider *vertSlider;

  QPushButton *buttonEnableVideo;
  QComboBox *comboboxVideoSource;

  QLabel *labelMotorRightSpeed;
  QLabel *labelMotorLeftSpeed;

  int cameraX;
  int cameraY;
  int motorRight;
  int motorLeft;

  QTimer *cameraAndSpeedTimer;
  QTime *cameraAndSpeedTime;
};

#endif
