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

 private:

};

#endif
