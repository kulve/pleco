#ifndef _IMU_H
#define _IMU_H

#include <QObject>

class Hardware;

class IMU : public QObject
{
  Q_OBJECT;

 public:
  // Constructor with the hardware specific plugin
  IMU(Hardware *hardware);

  // Desctructor
  ~IMU();

  // Start the IMU calculations
  bool enable(bool enable);

  // Returns yaw, pitch and roll as degrees (0-360)
  double *getYawPitchRoll(void);
  // Returns raw 9DoF values scaled to unsigned 8 bit values.
  QByteArray *get9DoFRaw(void);
  
  // Push 9DoF as scaled raw 8bit values and as doubles for IMU calculations
  // * raw8bit should be 0-255 values suitable for graphical presentations, 
  //   accuracy or actual scale is not an issue.
  // * data should be accurate raw data for accelerometers and magnetometers.
  //   The scale is not an issue as they will be normalized. Gyroscope values 
  //   must be in degrees/second.
  void pushSensorData(quint8 raw8bit[9], double data[9]);
  // pushYawPitchRoll() is an alternate for pushSensorData().
  // This push method can be used when the IMU calculations are done already
  // in the IMU unit and this IMU class must not do any calculations. In this
  // case the get9DoFRaw() method will always return zeros only.
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

  double gyroRawSum[3];
  double gyroRawOffset[3];
  bool gyroCalibrationDone;
  int gyroCalibrationTotal;
};

#endif
