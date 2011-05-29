#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <QObject>
#include <QString>

class Hardware : public QObject
{
  Q_OBJECT;

 public:
  virtual ~Hardware() {}

  // Hardware initialisation
  virtual bool init(void) = 0;

  //// Video related methods
  // Get video encoder name for GStreamer
  virtual QString getVideoEncoderName(void) const = 0;

  // TODO: IMU could be a separate module that's loaded by the hardware
  // module based on auto detection or configuration file

  //// IMU related methods 

  // Initialise IMU
  virtual bool initIMU(void) = 0;
  // Enable/disable the IMU, start reading the data
  virtual bool enableIMU(bool enable) = 0;

 signals:
  // Provide the 9 values for accelerometer, gyro and magnetometer
  // and the precision of the data in bytes (1,2,4 bytes for each value)
  void IMURaw(int accuracy_bytes, int data[9]);
};


#endif
