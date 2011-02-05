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
  bool init(void);
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

  quint32 getReg(quint32 addr);
  void setReg(quint32 addr, quint32 val);
  void powerOnServo(int timer);
  void powerOnMotor(int timer);
  void powerOffMotor(int timer);
  void powerOffServo(int timer);

  void *map;
  int fd;
  int motorRightSpeed;
  int motorLeftSpeed;
  int cameraXDegrees;
  int cameraYDegrees;

  bool enabled;
};

#endif
