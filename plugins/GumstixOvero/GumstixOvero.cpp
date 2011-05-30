#include "GumstixOvero.h"

#include <QtPlugin>



GumstixOvero::GumstixOvero(void)
{

}



bool GumstixOvero::init(void)
{

  return true;
}



QString GumstixOvero::getVideoEncoderName(void) const
{
  return "dsph263enc";
}



bool GumstixOvero::initIMU(IMU *imu)
{
  this->imu = imu;

  // We are using SparkFun 6DoF IMU + PNI11096 magnetometer and they
  // don't need any initialisation
  return true;
}



bool GumstixOvero::enableIMU(bool)
{
  // FIXME: start reading from 6DoF serial data from /dev/ttyO0
  // FIXME: start reading PNI from /dev/XXX

  return true;
}


Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOvero)
