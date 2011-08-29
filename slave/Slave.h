#ifndef _SLAVE_H
#define _SLAVE_H

#include "Transmitter.h"
#include "Hardware.h"
#include "Motor.h"
#include "Attitude.h"
#include "VideoSender.h"
#include "IMU.h"

#include <QCoreApplication>
#include <QTimer>

class Slave : public QCoreApplication
{
  Q_OBJECT;

 public:
  Slave(int &argc, char **argv);
  ~Slave();
  bool init(void);
  void connect(QString host, quint16 port);

 private slots:
  void readStats(void);
  void processError(QProcess::ProcessError error);
  void processStarted(void);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void updateCameraX(int degrees);
  void updateCameraY(int degrees);
  void updateMotorRight(int percent);
  void updateMotorLeft(int percent);
  void updateValue(quint8 type, quint16 value);
  void getImuData(void);
  void sendMeasurementsRate(int rate);

 private:
  void sendStats(void);

  Transmitter *transmitter;
  QProcess *process;
  QList<int> *stats;
  Motor *motor;
  VideoSender *vs;
  quint8 status;
  Hardware *hardware;
  IMU *imu;
  Attitude *attitude;
  QTimer *imuTimer;
};

#endif
