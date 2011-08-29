#ifndef _GENERIC_X86_H
#define _GENERIC_X86_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "Hardware.h"
#include "IMU.h"

class GenericX86 : public QObject, public Hardware
{
  Q_OBJECT;
  Q_INTERFACES(Hardware);

 public:
  GenericX86(void);
  ~GenericX86(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

  bool initIMU(IMU *imu);
  bool enableIMU(bool enable);

  void setMotorFrontRight(double power); /* 0 - 100 % */
  void setMotorFrontLeft(double power);  /* 0 - 100 % */
  void setMotorRearRight(double power);  /* 0 - 100 % */
  void setMotorRearLeft(double power);   /* 0 - 100 % */

 private slots:
  void generateData(void);

 private:
  QString encoderName;
  QTimer *dataTimer;
  IMU *imu;
};

#endif
