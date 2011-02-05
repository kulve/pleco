#include "Motor.h"


Motor::Motor(void):
  motorRightSpeed(0), motorLeftSpeed(0),
  cameraXDegrees(0), cameraYDegrees(0)
{
  qDebug() << "in" << __FUNCTION__;
  
}



Motor::~Motor(void)
{
  qDebug() << "in" << __FUNCTION__;

}



void Motor::shutdownAll(void)
{
  qDebug() << "in" << __FUNCTION__;

  motorRightEnable(false);
  motorLeftEnable(false);
  cameraXEnable(false);
  cameraYEnable(false);
}



void Motor::motorRight(int speed)
{
  qDebug() << "in" << __FUNCTION__;

  // Do nothing if value not changed
  if (motorRightSpeed == speed) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (speed == 0 && motorRightSpeed != 0) {
	motorRightEnable(false);
  }

  // Enable motor if value changed from zero to non-zero
  if (speed != 0 && motorRightSpeed == 0) {
	motorRightEnable(true);
  }

  motorRightSpeed = speed;

}




void Motor::motorLeft(int speed)
{
  qDebug() << "in" << __FUNCTION__;

  // Do nothing if value not changed
  if (motorLeftSpeed == speed) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (speed == 0 && motorLeftSpeed != 0) {
	motorLeftEnable(false);
  }

  // Enable motor if value changed from zero to non-zero
  if (speed != 0 && motorLeftSpeed == 0) {
	motorLeftEnable(true);
  }

  motorLeftSpeed = speed;
}




void Motor::cameraX(int degrees)
{
  qDebug() << "in" << __FUNCTION__;

  // Do nothing if value not changed
  if (cameraXDegrees == degrees) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (degrees == 0 && cameraXDegrees != 0) {
	cameraXEnable(false);
  }

  // Enable motor if value changed from zero to non-zero
  if (degrees != 0 && cameraXDegrees == 0) {
	cameraXEnable(true);
  }

  cameraXDegrees = degrees;
}




void Motor::cameraY(int degrees)
{
  qDebug() << "in" << __FUNCTION__;

  // Do nothing if value not changed
  if (cameraYDegrees == degrees) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (degrees == 0 && cameraXDegrees != 0) {
	cameraYEnable(false);
  }

  // Enable motor if value changed from zero to non-zero
  if (degrees != 0 && cameraYDegrees == 0) {
	cameraYEnable(true);
  }

  cameraYDegrees = degrees;
}




void Motor::motorRightEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;

}




void Motor::motorLeftEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;

}




void Motor::cameraXEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;

}




void Motor::cameraYEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;

}




