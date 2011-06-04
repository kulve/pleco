#include "IMU.h"
#include "Hardware.h"

#include <QObject>
#include <QDebug>
#include <QTime>

#include <math.h>     // sqrt, atan


#define Kp 2.0f       // proportional gain governs rate of convergence to accelerometer/magnetometer
#define Ki 0.005f     // integral gain governs rate of convergence of gyroscope biases


IMU::IMU(Hardware *hardware):
  QObject(), hardware(hardware), enabled(false),
  yaw(0), pitch(0), roll(0),
  q0(1), q1(0), q2(0), q3(0),
  exInt(0), eyInt(0), ezInt(0),
  lastUpdate(0), now(0),
  halfT(0)
{
  // FIXME: how to do this more nicely?
  accRaw[0]  = accRaw[1]  = accRaw[2]  = 0;
  gyroRaw[0] = gyroRaw[1] = gyroRaw[2] = 0;
  magnRaw[0] = magnRaw[1] = magnRaw[2] = 0;

  accRaw8bit[0]  = accRaw8bit[1]  = accRaw8bit[2]  = 0;
  gyroRaw8bit[0] = gyroRaw8bit[1] = gyroRaw8bit[2] = 0;
  magnRaw8bit[0] = magnRaw8bit[1] = magnRaw8bit[2] = 0;

  qDebug() << "In" << __FUNCTION__ << ", qX:" << q0 << q1 << q2 << q3;

}



IMU::~IMU()
{ 
  // Disable IMU, if still running
  if (enabled && !hardware->enableIMU(false)) {
	qWarning("Failed to stop IMU");
  }
}



bool IMU::enable(bool enable)
{

  qDebug() << "In" << __FUNCTION__ << ", Enable:" << enable;

  if (!hardware) {
	qWarning("No hardware plugin set!");
	return false;
  }

  // Enable IMU
  if (enable) {

	// Do nothing, if already enabled
	if (enabled) {
	  return true;
	}
	
	if (!hardware->initIMU(this)) {
	  qWarning("Failed to initialize IMU");
	  return false;
	}
	
	hardware->enableIMU(true);
	enabled = true;
	return true;
  }

  // Disable IMU

  // Do nothing if already disabled
  if (!enabled) {
	return true;
  }

  if (!hardware->enableIMU(false)) {
	qWarning("Failed to stop IMU");
	return false;
  }

  enabled = false;

  return true;
}



double *IMU::getYawPitchRoll(void)
{
  qDebug() << "In" << __FUNCTION__;

  double *data, *dp;

  data = new double[3];
  dp = data;

  *dp++ = yaw;
  *dp++ = pitch;
  *dp++ = roll;

  return data;
}




QByteArray *IMU::get9DoFRaw(void)
{
  qDebug() << "In" << __FUNCTION__;

  QByteArray *data;

  // One byte for each 9DoF value
  data = new QByteArray(9, 0); 
  
  // Scale the raw values to 1 byte (gyro values from millidegrees/sec to degrees/sec)
  (*data)[0] = accRaw8bit[0];
  (*data)[1] = accRaw8bit[1];
  (*data)[2] = accRaw8bit[2];
  (*data)[3] = gyroRaw8bit[0];
  (*data)[4] = gyroRaw8bit[1];
  (*data)[5] = gyroRaw8bit[2];
  (*data)[6] = magnRaw8bit[0];
  (*data)[7] = magnRaw8bit[1];
  (*data)[8] = magnRaw8bit[2];

  return data;
}



void IMU::pushSensorData(quint8 raw8bit[9], double data[9])
{

  qDebug() << "In" << __FUNCTION__ << ", data: " << data[0] << data[1] << data[2] << data[3] << data[4] << data[5] << data[6] << data[7] << data[8];

  // Accelerometer data
  accRaw[0] = data[0];
  accRaw[1] = data[1];
  accRaw[2] = data[2];

  // Gyroscope data
  gyroRaw[0] = data[3];
  gyroRaw[1] = data[4];
  gyroRaw[2] = data[5];

  // Magnetometer data
  magnRaw[0] = data[6];
  magnRaw[1] = data[7];
  magnRaw[2] = data[8];

  // 8bit Accelerometer data
  accRaw8bit[0] = raw8bit[0];
  accRaw8bit[1] = raw8bit[1];
  accRaw8bit[2] = raw8bit[2];

  // 8bit Gyroscope data
  gyroRaw8bit[0] = raw8bit[3];
  gyroRaw8bit[1] = raw8bit[4];
  gyroRaw8bit[2] = raw8bit[5];

  // 8bit Magnetometer data
  magnRaw8bit[0] = raw8bit[6];
  magnRaw8bit[1] = raw8bit[7];
  magnRaw8bit[2] = raw8bit[8];

  doIMUCalc();
}



void IMU::pushYawPitchRoll(double yaw, double pitch, double roll)
{
  this->yaw   = yaw;
  this->pitch = pitch;
  this->roll  = roll;
}



/*
 * Update yaw/pitch/roll
 */
void IMU::doIMUCalc()
{
  //qDebug() << "In" << __FUNCTION__;

  // Update yaw/pitch/roll
  /*
   * Taken from FreeIMU: http://www.varesano.net/projects/hardware/FreeIMU
   */
  double q[4]; // quaternion
  double gx, gy, gz; // estimated gravity direction
  getQ(q);

  //qDebug() << "In" << __FUNCTION__ << ", q: " << q[0] << q[1] << q[2] << q[3];

  gx = 2 * (q[1]*q[3] - q[0]*q[2]);
  gy = 2 * (q[0]*q[1] + q[2]*q[3]);
  gz = q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3];

  //qDebug() << "In" << __FUNCTION__ << ", gravity: " << gx << gy << gz;

  yaw = atan2(2 * q[1] * q[2] - 2 * q[0] * q[3], 2 * q[0]*q[0] + 2 * q[1] * q[1] - 1) * 180/M_PI;
  if (isnan(yaw)) {
	yaw = 0;
  }
  pitch = atan(gx / sqrt(gy*gy + gz*gz))  * 180/M_PI;
  if (isnan(pitch)) {
	pitch = 0;
  }
  roll = atan(gy / sqrt(gx*gx + gz*gz))  * 180/M_PI;
  if (isnan(roll)) {
	roll = 0;
  }

  qDebug() << "In" << __FUNCTION__ << ", yaw/pitch/roll:" << yaw << pitch << roll;
}


/*
 * Taken from FreeIMU: http://www.varesano.net/projects/hardware/FreeIMU
 */
//=====================================================================================================
// AHRS.c
// S.O.H. Madgwick
// 25th August 2010
//=====================================================================================================
// Description:
//
// Quaternion implementation of the 'DCM filter' [Mayhony et al].  Incorporates the magnetic distortion
// compensation algorithms from my filter [Madgwick] which eliminates the need for a reference
// direction of flux (bx bz) to be predefined and limits the effect of magnetic distortions to yaw
// axis only.
//
// User must define 'halfT' as the (sample period / 2), and the filter gains 'Kp' and 'Ki'.
//
// Global variables 'q0', 'q1', 'q2', 'q3' are the quaternion elements representing the estimated
// orientation.  See my report for an overview of the use of quaternions in this application.
//
// User must call 'AHRSupdate()' every sample period and parse calibrated gyroscope ('gx', 'gy', 'gz'),
// accelerometer ('ax', 'ay', 'ay') and magnetometer ('mx', 'my', 'mz') data.  Gyroscope units are
// radians/second, accelerometer and magnetometer units are irrelevant as the vector is normalised.
//
//=====================================================================================================
void IMU::AHRSupdate(double gx, double gy, double gz, double ax, double ay, double az, double mx, double my, double mz) {
  double norm;
  double hx, hy, hz, bx, bz;
  double vx, vy, vz, wx, wy, wz;
  double ex, ey, ez;

  //qDebug() << "In" << __FUNCTION__ << ", data: " << gx << gy << gz << ax << ay << az << mx << my << mz;

  //qDebug() << "In" << __FUNCTION__ << ", qX:" << q0 << q1 << q2 << q3;

  // auxiliary variables to reduce number of repeated operations
  double q0q0 = q0*q0;
  double q0q1 = q0*q1;
  double q0q2 = q0*q2;
  double q0q3 = q0*q3;
  double q1q1 = q1*q1;
  double q1q2 = q1*q2;
  double q1q3 = q1*q3;
  double q2q2 = q2*q2;   
  double q2q3 = q2*q3;
  double q3q3 = q3*q3;          
  
  //qDebug() << "In" << __FUNCTION__ << ", qXqY:" << q0q0 << q0q1 << q0q2 << q0q3 << q1q1 << q1q2 << q1q3 << q2q2 << q2q3 << q3q3;

  // normalise the measurements
  
  norm = sqrt(ax*ax + ay*ay + az*az);       
  if (norm == 0) {
	norm = 1;
  }
  ax = ax / norm;
  ay = ay / norm;
  az = az / norm;

  //qDebug() << "In" << __FUNCTION__ << ", acc:" << norm << ax << ay << az;

  // Get current time in milliseconds
  QTime time = QTime::currentTime();
  now = time.hour() * 3600 * 1000 + time.minute() * 60 * 1000 + time.second() * 1000 + time.msec();

  // Estimate the lastUpdate if running this for the first time
  if (lastUpdate == 0) {
	lastUpdate = now - 10;
  }

  // If midnight just passed by, calculate the correct halfT
  if (now < lastUpdate) {
	halfT = (((24 * 3600 * 1000) - lastUpdate) + now) / 2000.0;
  } else {
	halfT = (now - lastUpdate) / 2000.0;
  }
  lastUpdate = now;

  //qDebug() << "In" << __FUNCTION__ << ", halfT:" << halfT;

  norm = sqrt(mx*mx + my*my + mz*mz);          
  if (norm == 0) {
	norm = 1;
  }
  mx = mx / norm;
  my = my / norm;
  mz = mz / norm;

  //qDebug() << "In" << __FUNCTION__ << ", magn:" << mx << my << mz;

  // compute reference direction of flux
  hx = 2*mx*(0.5 - q2q2 - q3q3) + 2*my*(q1q2 - q0q3) + 2*mz*(q1q3 + q0q2);
  hy = 2*mx*(q1q2 + q0q3) + 2*my*(0.5 - q1q1 - q3q3) + 2*mz*(q2q3 - q0q1);
  hz = 2*mx*(q1q3 - q0q2) + 2*my*(q2q3 + q0q1) + 2*mz*(0.5 - q1q1 - q2q2);         
  bx = sqrt((hx*hx) + (hy*hy));
  bz = hz;     

  //qDebug() << "In" << __FUNCTION__ << ", direction of flux:" << hx << hy << hz;
  
  // estimated direction of gravity and flux (v and w)
  vx = 2*(q1q3 - q0q2);
  vy = 2*(q0q1 + q2q3);
  vz = q0q0 - q1q1 - q2q2 + q3q3;
  wx = 2*bx*(0.5 - q2q2 - q3q3) + 2*bz*(q1q3 - q0q2);
  wy = 2*bx*(q1q2 - q0q3) + 2*bz*(q0q1 + q2q3);
  wz = 2*bx*(q0q2 + q1q3) + 2*bz*(0.5 - q1q1 - q2q2);  
  
  //qDebug() << "In" << __FUNCTION__ << ", direction of grav:" << vx << vy << vz;
  //qDebug() << "In" << __FUNCTION__ << ", direction of flux:" << wx << wy << wz;

  // error is sum of cross product between reference direction of fields and direction measured by sensors
  ex = (ay*vz - az*vy) + (my*wz - mz*wy);
  ey = (az*vx - ax*vz) + (mz*wx - mx*wz);
  ez = (ax*vy - ay*vx) + (mx*wy - my*wx);

  //qDebug() << "In" << __FUNCTION__ << ", sensor error:" << ex << ey << ez;
  
  // integral error scaled integral gain
  exInt = exInt + ex*Ki;
  eyInt = eyInt + ey*Ki;
  ezInt = ezInt + ez*Ki;
  
  //qDebug() << "In" << __FUNCTION__ << ", integral error:" << exInt << eyInt << ezInt;

  // adjusted gyroscope measurements
  gx = gx + Kp*ex + exInt;
  gy = gy + Kp*ey + eyInt;
  gz = gz + Kp*ez + ezInt;

  //qDebug() << "In" << __FUNCTION__ << ", gyro:" << gx << gy << gz;

  // integrate quaternion rate and normalise
  q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfT;
  q1 = q1 + (q0*gx + q2*gz - q3*gy)*halfT;
  q2 = q2 + (q0*gy - q1*gz + q3*gx)*halfT;
  q3 = q3 + (q0*gz + q1*gy - q2*gx)*halfT;  
  
  //qDebug() << "In" << __FUNCTION__ << ", qX1:" << q0 << q1 << q2 << q3;

  // normalise quaternion
  norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
  if (norm == 0) {
	norm = 1;
  }
  q0 = q0 / norm;
  q1 = q1 / norm;
  q2 = q2 / norm;
  q3 = q3 / norm;

  //qDebug() << "In" << __FUNCTION__ << ", qX2:" << q0 << q1 << q2 << q3;
}



/*
 * Taken from FreeIMU: http://www.varesano.net/projects/hardware/FreeIMU
 */
void IMU::getQ(double * q) {
  
  // gyro values are expressed in millidegrees/sec, the * M_PI/180 will convert it to radians/sec
  AHRSupdate((gyroRaw[0] / (double)1000) * M_PI/180,
			 (gyroRaw[1] / (double)1000) * M_PI/180,
			 (gyroRaw[2] / (double)1000) * M_PI/180,
			 accRaw[0], accRaw[1], accRaw[2],
			 magnRaw[0], magnRaw[1], magnRaw[2]);
  q[0] = q0;
  q[1] = q1;
  q[2] = q2;
  q[3] = q3;
}
