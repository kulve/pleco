#ifndef _GUMSTIX_OVERO_H
#define _GUMSTIX_OVERO_H

#include <QObject>
#include <QString>
#include <QtPlugin>

#include "../../common/Hardware.h"
#include "../../slave/IMU.h"

class GumstixOvero : public QObject, public Hardware
{
  Q_OBJECT;
  Q_INTERFACES(Hardware);

 public:
  GumstixOvero(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

  bool initIMU(IMU *imu);
  bool enableIMU(bool);

 private:
  IMU *imu;
};

#endif
