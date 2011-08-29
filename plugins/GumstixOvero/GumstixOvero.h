#ifndef _GUMSTIX_OVERO_H
#define _GUMSTIX_OVERO_H

#include <QObject>
#include <QString>
#include <QtPlugin>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>

#include "../../common/Hardware.h"
#include "../../common/IMU.h"

class GumstixOvero : public QObject, public Hardware
{
  Q_OBJECT;
  Q_INTERFACES(Hardware);

 public:
  GumstixOvero(void);
  ~GumstixOvero(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

  bool initIMU(IMU *imu);
  bool enableIMU(bool);

  void setMotorFrontRight(double power); /* 0 - 100 % */
  void setMotorFrontLeft(double power);  /* 0 - 100 % */
  void setMotorRearRight(double power);  /* 0 - 100 % */
  void setMotorRearLeft(double power);   /* 0 - 100 % */

 private slots:
  void readPendingSerialData();
  void readPendingPniData();

 private:
  bool openSerialDevice(void);
  bool openPniDevice(void);
  void parseSerialData(void);
  void parsePniData(void);
  void setMotor(QFile &pwm, double power);

  IMU *imu;
  int serialFD;
  QString serialPortName;
  QTcpSocket serialPort;
  QByteArray serialData;
  bool imuRead;
  int pniFD;
  QString pniFileName;
  QTcpSocket pniDevice;
  QByteArray pniData;
  bool pniRead;
  double magn[3];
  double ins[9];
  quint8 raw8bit[9];

  QFile pwm8;
  QFile pwm9;
  QFile pwm10;
  QFile pwm11;
};

#endif
