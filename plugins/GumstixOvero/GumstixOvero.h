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
  void readPendingSerialData();
  void readPendingPniData();

 private:
  bool openSerialDevice(void);
  bool openPniDevice(void);
  void parseSerialData(void);
  void parsePniData(void);

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
};

#endif
