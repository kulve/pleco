#ifndef _PIDCONTROL_H
#define _PIDCONTROL_H

#include <QObject>

/*
 * From:
 * http://www.linuxpcrobot.org/?q=node/4
 */

class PIDControl: public QObject
{
  Q_OBJECT;

public:

  PIDControl(void);
  ~PIDControl(void);

  double CalcPID(double scale=1);
  void SetTarget(double target);
  void SetActual(double actual);
  void SetDifGain(double gain);
  void SetIntGain(double gain);
  void SetErrGain(double gain);
  void SetBias(int bias);
  void SetAccel(double accel);
  void SetMin(int min);
  void SetMax(int max);
#if 0
  double GetDifGain(void);
  double GetIntGain(void);
  double GetErrGain(void);
  int GetBias(void);
  double GetAccel(void);
  int GetMin(void);
  int GetMax(void);
  void Clear(void);
#endif

protected:

  // Use this when the CalcPID is called
  double target;
  double actual;

  // Settable/readable parameters
  double m_gain_dif;     // Gain for differential
  double m_gain_int;     // Gain for integral
  double m_gain_error;   // Gain for error
  double m_acceleration; // Acceleration 

  int m_bias;            // Offset bias for algorithm
  int m_pidMax;          // Maximum output value
  int m_pidMin;          // Minimum output value

  // Operational variables
  double m_error;        // The proportional error
  double m_integral;     // The integral value
  double m_differential; // The differential value
};

#endif
