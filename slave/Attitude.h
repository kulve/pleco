#ifndef _ATTITUDE_H
#define _ATTITUDE_H

#include "PID.h"

#include <QObject>
#include <QDebug>

class Hardware;
class IMU;

typedef enum flyMode {
  FLY_MODE_ON_GROUND,
  FLY_MODE_TAKE_OFF,
  FLY_MODE_STAND_BY,
  FLY_MODE_ON_AIR,
  FLY_MODE_LANDING
} FLY_MODE;

class Attitude: public QObject
{
  Q_OBJECT;

 public:
  Attitude(Hardware *hardware);
  ~Attitude(void);

  // Navigational commands
  void setHeading(double heading);       // In degrees
  void setSpeed(double speed);           // In percent
  void adjustHeight(double height);      // In meters, relatively according to air pressure

  // Low level attitude commands
  void setYaw(double yaw);
  void setPitch(double pitch);
  void setRoll(double roll);

  // Take off and landing
  void takeOff(void);                    // Take off from ground
  void land(void);                       // Land from air to ground

  // Misc. methods
  IMU *getImu(void);

#if 0
 signals:
  void status(void); // FIXME: send status info to GUI
#endif

 private slots:
  // Inform about the real values
  void updateAttitude(double yaw, double pitch, double roll);
  void updateSonar(double sonar);     // In meters

 private:
  void clampMotorPower(double *power);
  PID yaw;
  PID pitch;
  PID roll;
  PID sonar;   // For take off, stand by, and landing
  //PIDControl height;// For on air
  
  // Yaw, pitch and roll calculations
  IMU *imu;

  // Motors, 0-100%
  double motorFrontRight;
  double motorFrontLeft;
  double motorRearRight;
  double motorRearLeft;
  
  FLY_MODE mode;

  Hardware *hardware;
};

#endif
