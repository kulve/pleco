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

 private slots:
  void readPendingData();
  void readPNI();

 private:
  bool openSerialDevice(QString device);
  void parseData(void);

  IMU *imu;
  int fd;
  QString serialPortName;
  QTcpSocket serialPort;
  QByteArray data;
  QFile pni11096;
  QTimer *pniTimer;
  bool pniRead;
  double magn[3];
};

#endif
