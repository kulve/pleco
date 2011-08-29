/*
 * From:
 * http://www.linuxpcrobot.org/?q=node/4
 */

#include "PIDControl.h"

#include <QObject>
#include <QDebug>

#include "limits.h"       // INT_MIN/MAX

PIDControl::PIDControl()
{
  target = 0;         // Target for the CalcPID
  actual = 0;         // Actual for the CalcPID
  m_gain_error = 1;   // Gain for error
  m_gain_int = 1;     // Gain for integral
  m_gain_dif = 1;     // Gain for differential
  m_bias = 0;         // Offset bias for control
  m_pidMax = INT_MAX; // Output clamp, max value
  m_pidMin = INT_MIN; // Output clamp, min value
  m_acceleration = 0; // Boost for acceleration
  m_error = 0;        // The proportional error
  m_integral = 0;     // The integral value
  m_differential = 0; // The differential value
}



PIDControl::~PIDControl(void)
{
  // Nothing here
}



double PIDControl::CalcPID(double scale)
{ 
  //qDebug() << __FUNCTION__ << ", target:" << target << ", actual:" << actual << ", scale:" << scale;

  // Calculate positional error
  double error = target - actual;

  // Calculate the differential between last error 
  m_differential = error - m_error; 

  // Update integral with new error 
  m_integral += error; 

  // Save the last last error
  m_error = error;

  // Sum the raw PID components
  double rawpid=((error*m_gain_error)+(m_integral*m_gain_int) +
				 (m_differential*m_gain_dif))*scale;

  // Adjust for acceleration and bias
  double pid = m_bias + rawpid + m_acceleration;

  // Return clamped PID value
  return (pid > m_pidMax) ? m_pidMax : 
	(pid < m_pidMin) ? m_pidMin : pid;
}



void PIDControl::SetTarget(double target)
{
  target = target;
}



void PIDControl::SetActual(double actual)
{
  actual = actual;
}



void PIDControl::SetDifGain(double gain)
{
  m_gain_dif = gain;
}




void PIDControl::SetIntGain(double gain)
{
  m_gain_int = gain;
}




void PIDControl::SetErrGain(double gain)
{
  m_gain_error = gain;
}




void PIDControl::SetBias(int bias)
{
  m_bias = bias;
}




void PIDControl::SetAccel(double accel)
{
  m_acceleration = accel;
}




void PIDControl::SetMin(int min)
{
  m_pidMin = min;
}




void PIDControl::SetMax(int max)
{
  m_pidMax = max;
}
