#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "../slave/IMU.h"

#include <QString>
#include <QtPlugin>

class Hardware
{

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

  // Initialise IMU sensors, give IMU pointer for pushing sensor data
  virtual bool initIMU(IMU *imu) = 0;
  // Enable/disable the IMU, start reading the data
  virtual bool enableIMU(bool enable) = 0;
};

Q_DECLARE_INTERFACE(Hardware, "Hardware");


#endif
