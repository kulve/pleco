#ifndef _MOTOR_H
#define _MOTOR_H

#include <QObject>
#include <QDebug>

class Motor: public QObject
{
  Q_OBJECT;

 public:
  Motor(void);
  ~Motor(void);
  void motorRight(int speed);
  void motorLeft(int speed);
  void cameraX(int degrees);
  void cameraY(int degrees);

 private slots:

 private:
  void motorRightEnable(bool enable);
  void motorLeftEnable(bool enable);
  void cameraXEnable(bool enable);
  void cameraYEnable(bool enable);

  int motorRightSpeed;
  int motorLeftSpeed;
  int cameraXDegrees;
  int cameraYDegrees;
};

#endif
