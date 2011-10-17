#include "Attitude.h"
#include "PID.h"
#include "Hardware.h"
#include "IMU.h"

#include <QObject>
#include <QDebug>


Attitude::Attitude(Hardware *hardware):
  yaw(), pitch(), roll(), sonar(),
  imu(NULL), motorFrontRight(0), motorFrontLeft(0), motorRearRight(0), motorRearLeft(0),
  mode(FLY_MODE_ON_GROUND), hardware(hardware)
{

  // Set PID limits for yaw, pitch, roll, and sonar
  yaw.SetOutputLimits(-40, 40);
  pitch.SetOutputLimits(-40, 40);
  roll.SetOutputLimits(-40, 40);
  sonar.SetOutputLimits(-40, 40);

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
  double yawChange = yaw.Compute();
  double pitchChange = pitch.Compute();
  double rollChange = roll.Compute();
#endif
  // FIXME: mixing!!!!
  double rollChange = pitch.Compute();
  if (rollChange < 0) {
	qDebug() << __FUNCTION__ << ": NEGATIVE rollChange:" << rollChange;
	return;
  }

  qDebug() << __FUNCTION__ << ": rollChange:" << rollChange;

  // Assuming Compute returns values -100 - 100 % telling the change in motor power
#ifdef USE_ALL_MOTORS
  double positiveYawChange   = +1 * yawChange;
  double negativeYawChange   = -1 * yawChange;
  double positivePitchChange = +1 * pitchChange;
  double negativePitchChange = -1 * pitchChange;
#endif
#if 0
  double positiveRollChange  = +1 * rollChange;
  double negativeRollChange  = -1 * rollChange;

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

  // We set absolute (average) values from the PIDs
  double motorFrontLeftChange   = 0;
  double motorRearRightChange   = 0;
  double motorFrontRightChange  = 0;
  double motorRearLeftChange    = 0;
  int sensorsInUse              = 0;
#endif
#ifdef USE_ALL_MOTORS
  // Yaw  : Increase CCW rotating motors, decrease CW rotating motors
  motorFrontLeftChange  += positiveYawChange;
  motorRearRightChange  += positiveYawChange;
  motorFrontRightChange += negativeYawChange;
  motorRearLeftChange   += negativeYawChange;
  sensorsInUse          += 1;

  // Pitch: Increase front motors, decrease rear motors
  motorFrontLeftChange  += positivePitchChange;
  motorFrontRightChange += positivePitchChange;
  motorRearLeftChange   += negativePitchChange;
  motorRearRightChange  += negativePitchChange;
  sensorsInUse          += 1;

#endif
#if 0
  // Roll : Increase left motors, decrease right motors
  motorFrontLeftChange  += positiveRollChange;
  motorRearLeftChange   += positiveRollChange;
  motorFrontRightChange += negativeRollChange;
  motorRearRightChange  += negativeRollChange;
  sensorsInUse          += 1;

  // Calculate average
  motorFrontLeftChange  /= sensorsInUse;
  motorRearLeftChange   /= sensorsInUse;
  motorFrontRightChange /= sensorsInUse;
  motorRearRightChange  /= sensorsInUse;

  motorFrontRight       += motorFrontRightChange;
  motorFrontLeft        += motorFrontLeftChange;
  motorRearRight        += motorRearRightChange;
  motorRearLeft         += motorRearLeftChange;
#endif

  if (rollChange > 0) {
	motorFrontRight = rollChange + 20;
	motorFrontLeft = 20 - rollChange;
  } else {
	motorFrontLeft = -1 * rollChange + 20;
	motorFrontRight = 20 + rollChange;
  }

  clampMotorPower(&motorFrontRight);
  clampMotorPower(&motorFrontLeft);
  clampMotorPower(&motorRearRight);
  clampMotorPower(&motorRearLeft);

  qDebug() << __FUNCTION__ << "angle/left/right/roll," << pitchDegrees << "," << motorFrontLeft << "," << motorFrontRight << "," << rollChange;

  // Update motors
  hardware->setMotorFrontRight(motorFrontRight);
  //hardware->setMotorFrontLeft(motorFrontLeft);
#ifdef USE_ALL_MOTORS
  hardware->setMotorRearRight(motorRearRight);
  hardware->setMotorRearLeft(motorRearLeft);
#endif
}

void Attitude::clampMotorPower(double *power)
{
  if (*power < 10)  *power = 10;
  if (*power > 70)  *power = 70;
}

void Attitude::updateSonar(double height)
{
  sonar.SetActual(height);
  double heightChange = sonar.Compute();

  motorFrontRight += heightChange;
  motorFrontLeft  += heightChange;
  motorRearRight  += heightChange;
  motorRearLeft   += heightChange;

  clampMotorPower(&motorFrontRight);
  clampMotorPower(&motorFrontLeft);
  clampMotorPower(&motorRearRight);
  clampMotorPower(&motorRearLeft);

  // Update motors
  hardware->setMotorFrontRight(motorFrontRight);
  hardware->setMotorFrontLeft(motorFrontLeft);
  hardware->setMotorRearRight(motorRearRight);
  hardware->setMotorRearLeft(motorRearLeft);
}




