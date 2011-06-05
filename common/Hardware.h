#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "IMU.h"

#include <QString>
#include <QtPlugin>

class Hardware
{

 public:
  virtual ~Hardware() {}

  // Hardware initialisation
  virtual bool init(void) = 0;

  /*** Video related methods ***/

  // Get video encoder name for GStreamer
  virtual QString getVideoEncoderName(void) const = 0;

  /*** IMU related methods ***/

  // TODO: IMU could be a separate plugin that's loaded by the hardware
  // plugin based on auto detection or configuration file

  // See pushSensorData() and pushYawPitchRoll() in IMU class for
  // pushing the IMU sensor data from the hardware specific plugin to IMU
  // class.

  // Initialise IMU sensors, give IMU pointer for pushing sensor data
  virtual bool initIMU(IMU *imu) = 0;

  // Enable/disable the IMU sensors, start pushing the sensor data to IMU class
  virtual bool enableIMU(bool enable) = 0;
};

Q_DECLARE_INTERFACE(Hardware, "Hardware");


#endif
