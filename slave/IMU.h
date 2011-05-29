#ifndef _IMU_H
#define _IMU_H

#include "Hardware.h"

#include <QObject>

class IMU : public QObject
{
  Q_OBJECT;

 public:
  IMU(Hardware *hardware);
  ~IMU();
  bool enable(bool enable);
  QByteArray *get9DoF(int accuracy_bytes);
  QByteArray *get9DoFRaw(int accuracy_bytes);

 private slots:
  void doIMUCalc(int accuracy_bytes, int data[9]);

 private:
  Hardware *hardware;
  bool enabled;
  int acc[3];
  int gyro[3];
  int magn[3];
  int accRaw[3];
  int gyroRaw[3];
  int magnRaw[3];
};

#endif
