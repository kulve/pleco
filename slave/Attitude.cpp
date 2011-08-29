#include "Attitude.h"
#include "PIDControl.h"
#include "Hardware.h"
#include "IMU.h"

#include <QObject>
#include <QDebug>


Attitude::Attitude(Hardware *hardware):
  yaw(), pitch(), roll(), sonar(),
  imu(NULL), motorFrontRight(0), motorFrontLeft(0), motorRearRight(0), motorRearLeft(0),
  mode(FLY_MODE_ON_GROUND), hardware(hardware)
{

  // Set PID values for yaw, pitch, roll, and sonar
  yaw.SetDifGain(1);
  yaw.SetIntGain(1);
  yaw.SetErrGain(1);
  yaw.SetBias(1);
  yaw.SetAccel(1);
  yaw.SetMin(-100);
  yaw.SetMax(100); // FIXME: percents?

  pitch.SetDifGain(1);
  pitch.SetIntGain(1);
  pitch.SetErrGain(1);
  pitch.SetBias(1);
  pitch.SetAccel(1);
  pitch.SetMin(-100);
  pitch.SetMax(100); // FIXME: percents?

  roll.SetDifGain(1);
  roll.SetIntGain(1);
  roll.SetErrGain(1);
  roll.SetBias(1);
  roll.SetAccel(1);
  roll.SetMin(-100);
  roll.SetMax(100); // FIXME: percents?

  sonar.SetDifGain(1);
  sonar.SetIntGain(1);
  sonar.SetErrGain(1);
  sonar.SetBias(1);
  sonar.SetAccel(1);
  sonar.SetMin(-100);
  sonar.SetMax(100); // FIXME: percents?

  imu = new IMU(hardware);
  QObject::connect(imu, SIGNAL(newAttitude(double, double, double)), this, SLOT(updateAttitude(double, double, double)));
  imu->enable(true);

}



Attitude::~Attitude(void)
{

}



IMU *Attitude::getImu(void)
{
  return imu;
}



void Attitude::setHeading(double heading)
{
  setYaw(heading);
}


void Attitude::setSpeed(double speed)
{
  int max_speed_degrees = 45;
  
  // Set pitch according to speed percentage
  setPitch(speed * max_speed_degrees);
}


void Attitude::adjustHeight(double /*height*/)
{
  // FIXME: set height target in meters based on air pressure
}



void Attitude::setYaw(double degrees)
{
  yaw.SetTarget(degrees);
}


void Attitude::setPitch(double degrees)
{
  pitch.SetTarget(degrees);
}


void Attitude::setRoll(double degrees)
{
  roll.SetTarget(degrees);
}



void Attitude::takeOff(void)
{
  // FIXME: take off!!!!
}


void Attitude::land(void)
{
  // FIXME: land
}


#if 0
void Attitude::status(void)
{
  // FIXME: send something
}
#endif


void Attitude::updateAttitude(double yawDegrees, double pitchDegrees, double rollDegrees)
{
  yaw.SetActual(yawDegrees);
  pitch.SetActual(pitchDegrees);
  roll.SetActual(rollDegrees);

#undef USE_ALL_MOTORS
#ifdef USE_ALL_MOTORS
  double yawFix = yaw.CalcPID();
  double pitchFix = pitch.CalcPID();
#endif
  double rollFix = roll.CalcPID();

  qDebug() << __FUNCTION__ << ": rollFix:" << rollFix;

  // Assuming CalcPID returns values -100 - 100 % telling the change in motor power
  // Convert to scale (50% == 0.5, etc)
#ifdef USE_ALL_MOTORS
  double positiveYawFix   = 1 + yawFix   / 100;
  double negativeYawFix   = 1 - yawFix   / 100;
  double positivePitchFix = 1 + pitchFix / 100;
  double negativePitchFix = 1 - pitchFix / 100;
#endif
  double positiveRollFix  = 1 + rollFix  / 100;
  double negativeRollFix  = 1 - rollFix  / 100;

  /* Calculate new speeds for all motors
   * o  o
   *  \/      
   *  /\
   * o  o
   *
   * CW : frontLeft, rearRight
   * CCW: frontRight, rearLeft
   * Motors are in 45 degree angles but as they are all the same, we don't need to scale them
   */

#ifdef USE_ALL_MOTORS
  // Yaw  : Increase CCW rotating motors, decrease CW rotating motors
  motorFrontLeft  *= positiveYawFix;
  motorRearRight  *= positiveYawFix;
  motorFrontRight *= negativeYawFix;
  motorRearLeft   *= negativeYawFix;

  // Pitch: Increase front motors, decrease rear motors
  motorFrontLeft  *= positivePitchFix;
  motorFrontRight *= positivePitchFix;
  motorRearLeft   *= negativePitchFix;
  motorRearRight  *= negativePitchFix;
#endif
  // Roll : Increase left motors, decrease right motors
  motorFrontLeft  *= positiveRollFix;
  motorRearLeft   *= positiveRollFix;
  motorFrontRight *= negativeRollFix;
  motorRearRight  *= negativeRollFix;

  // Update motors
  hardware->setMotorFrontRight(motorFrontRight);
  hardware->setMotorFrontLeft(motorFrontLeft);
#ifdef USE_ALL_MOTORS
  hardware->setMotorRearRight(motorRearRight);
  hardware->setMotorRearLeft(motorRearLeft);
#endif
}


void Attitude::updateSonar(double height)
{
  sonar.SetActual(height);
  double heightFix = sonar.CalcPID();

  // Assuming CalcPID returns values -100 - 100 % telling the change in motor power
  // Convert to scale (50% == 0.5, etc)
  heightFix = 1 + heightFix / 100;

  motorFrontRight *= heightFix;
  motorFrontLeft  *= heightFix;
  motorRearRight  *= heightFix;
  motorRearLeft   *= heightFix;

  // Update motors
  hardware->setMotorFrontRight(motorFrontRight);
  hardware->setMotorFrontLeft(motorFrontLeft);
  hardware->setMotorRearRight(motorRearRight);
  hardware->setMotorRearLeft(motorRearLeft);
}




