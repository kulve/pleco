#ifndef _IMU_H
#define _IMU_H

#include <QObject>

class Hardware;

class IMU : public QObject
{
  Q_OBJECT;

 public:
  IMU(Hardware *hardware);
  ~IMU();
  bool enable(bool enable);
  double *getYawPitchRoll(void);
  QByteArray *get9DoFRaw(void);
  
  /* Push 9DoF as 8bit values for graphs etc, and as doubles for match */
  void pushSensorData(quint8 raw8bit[9], double data[9]);
  /* Push already filtered yaw/pitch/roll values */
  void pushYawPitchRoll(double yaw, double pitch, double roll);


 private slots:
  void doIMUCalc(void);

 private:
  void getQ(double *q);
  void AHRSupdate(double gx, double gy, double gz, double ax, double ay, double az, double mx, double my, double mz);

  Hardware *hardware;
  bool enabled;

  double accRaw[3];
  double gyroRaw[3];
  double magnRaw[3];

  quint8 accRaw8bit[3];
  quint8 gyroRaw8bit[3];
  quint8 magnRaw8bit[3];

  double yaw;
  double pitch;
  double roll;

  double q0, q1, q2, q3; // quaternion elements representing the estimated orientation
  double exInt, eyInt, ezInt;  // scaled integral error
  int lastUpdate, now; // sample period expressed in milliseconds
  double halfT; // half the sample period expressed in seconds
};

#endif
