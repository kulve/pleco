#ifndef _GENERIC_X86_H
#define _GENERIC_X86_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "../../common/Hardware.h"
#include "../../slave/IMU.h"

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

 private slots:
  void generateData(void);

 private:
  QString encoderName;
  QTimer *dataTimer;
  IMU *imu;
};

#endif
