/**********************************************************************************************
 * Arduino PID Library - Version 1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Code is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
 **********************************************************************************************/

#include <PID.h>
#include <QTime>
#include <QDebug>
#include <stdlib.h>   // getenv for KP, KD, KI

/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up 
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/
PID::PID()
{
  PID::SetOutputLimits(0, 255);				//default output limit corresponds to
                                            //the arduino pwm limits

  SampleTime = 10;							//default Controller Sample Time is 0.01 seconds

  PID::SetControllerDirection(DIRECT);
  PID::AdaptTunings();

  lastTime = 0;
  ITerm = 0;
  lastInput = 0;
  lastOutput = 0;
  target = 0;
  actual = 0;
  historyFull = false;

  pastIndex = 0;
  for (int i = 0; i < HISTORY_LEN; ++i) {
	pastInputs[i] = 0;
	pastTimes[i] = 0;
  }

  inAuto = true;
}
 
 
/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed
 **********************************************************************************/ 
double PID::Compute()
{
  // Get current time in milliseconds
  QTime time = QTime::currentTime();
  unsigned long now = time.hour() * 3600 * 1000 + time.minute() * 60 * 1000 + time.second() * 1000 + time.msec();

  // Estimate the lastTime if running this for the first time
  if (lastTime == 0) {
	lastTime = now - SampleTime;
  }

  int timeChange;
  // If midnight just passed by, calculate the correct timeChange
  if (now < lastTime) {
	timeChange = ((24 * 3600 * 1000) - lastTime) + now;
  } else {
	timeChange = now - lastTime;
  }
  lastTime = now;

  if(!inAuto || !historyFull) return 0;

  /*Compute all the working error variables*/
  double error = target - actual;

  qDebug() << __FUNCTION__ << ", Time change:" << timeChange << ", in/out/error:" << actual << output << error;

  // Try to adapt the tunings based on the error
  //AdaptTunings(error);

  ITerm+= (ki * error);
  if(ITerm > outMax) ITerm= outMax;
  else if(ITerm < outMin) ITerm= outMin;

  double dtimeChange;
  double dInput;
  if (HISTORY_LEN > 1) {
	int index = pastIndex - 1;
	if (index == -1) {
	  index = HISTORY_LEN - 1;
	}
	dtimeChange = (pastTimes[index] - pastTimes[pastIndex]) / (double)HISTORY_LEN;
	dInput = ((pastInputs[index] - pastInputs[pastIndex]) / (double)HISTORY_LEN) * (dtimeChange / (double)1000);
  } else {
	dtimeChange = timeChange;
	dInput = pastInputs[0] * (dtimeChange / (double)1000);
  }


  // HACK: sanity check, do nothing, if over 250ms since last update (I'm assuming this is some sort of error)
  if (dtimeChange > 50) return 0;

  /*Compute PID Output*/
  output = kp * error + ITerm - kd * dInput;

  if(output > outMax) output = outMax;
  else if(output < outMin) output = outMin;

  qDebug() << __FUNCTION__ << " PID," << kp*error << "," << ITerm << "," << kd*dInput << "," << output << "," << actual << "," << target << "," << dtimeChange;
  
  /*Remember some variables for next time*/
  lastInput = actual;
  lastOutput = output;


  return output;
}

void PID::SetTarget(double target)
{
  this->target = target;
}

void PID::SetActual(double actual)
{

  // Get current time in milliseconds
  QTime time = QTime::currentTime();
  unsigned long now = time.hour() * 3600 * 1000 + time.minute() * 60 * 1000 + time.second() * 1000 + time.msec();

  this->actual = actual;

  this->pastInputs[pastIndex] = actual;
  this->pastTimes[pastIndex] = now;
  pastIndex++;
  if (pastIndex == HISTORY_LEN) {
	if (historyFull == false) {
	  historyFull = true;
	}
	pastIndex = 0;
  }
}

void PID::AdaptTunings(double error)
{
  if (error < 0) {
	error *= -1;
  }

#if 0
  // Use conservative tuning values if we are below the low mark and
  // aggressive values if we are above the high mark
  if (error < ADAPTIVE_TUNING_LOW_THRESHOLD ) {
	SetTunings(CONSERVATIVE_TUNING_KP, CONSERVATIVE_TUNING_KI, CONSERVATIVE_TUNING_KD);
  } else if (error > ADAPTIVE_TUNING_HIGH_THRESHOLD ) {
	SetTunings(AGGRESSIVE_TUNING_KP, AGGRESSIVE_TUNING_KI, AGGRESSIVE_TUNING_KD);
  }
#else
  char *skp = getenv("KP");
  char *ski = getenv("KI");
  char *skd = getenv("KD");
  if (skp) {
	SetTunings(atof(skp), atof(ski), atof(skd));
  } else {
	SetTunings(CONSERVATIVE_TUNING_KP, CONSERVATIVE_TUNING_KI, CONSERVATIVE_TUNING_KD);
  }
#endif
}

/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted. 
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/ 
void PID::SetTunings(double Kp, double Ki, double Kd)
{
  if (Kp<0 || Ki<0 || Kd<0) return;

  dispKp = Kp; dispKi = Ki; dispKd = Kd;

  // FIXME: use actual SampleTime?
  double SampleTimeInSec = ((double)SampleTime)/(double)1000;  
  kp = Kp;
  ki = Ki * SampleTimeInSec;
  kd = Kd / SampleTimeInSec;
 
  if(controllerDirection ==REVERSE)
	{
      kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
	}

  qDebug() << __FUNCTION__ << "kp:" << kp << ", ki: " << ki << ", kd:" << kd;

}
  
/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed	
 ******************************************************************************/
void PID::SetSampleTime(int NewSampleTime)
{
  if (NewSampleTime > 0)
	{
      double ratio  = (double)NewSampleTime
		/ (double)SampleTime;
      ki *= ratio;
      kd /= ratio;
      SampleTime = (unsigned long)NewSampleTime;
	}
}
 
/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void PID::SetOutputLimits(double Min, double Max)
{
  if(Min >= Max) return;
  outMin = Min;
  outMax = Max;
 
  if(inAuto)
	{
	  if(output > outMax) output = outMax;
	  else if(output < outMin) output = outMin;
	 
	  if(ITerm > outMax) ITerm= outMax;
	  else if(ITerm < outMin) ITerm= outMin;
	}
}

/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (non-zero)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/ 
void PID::SetMode(int Mode)
{
  bool newAuto = (Mode == AUTOMATIC);
  if(newAuto == !inAuto)
    {  /*we just went from manual to auto*/
	  PID::Initialize();
    }
  inAuto = newAuto;
}
 
/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/ 
void PID::Initialize()
{
  ITerm = output;
  lastInput = actual;
  if(ITerm > outMax) ITerm = outMax;
  else if(ITerm < outMin) ITerm = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads 
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void PID::SetControllerDirection(int Direction)
{
  if(inAuto && Direction !=controllerDirection)
	{
	  kp = (0 - kp);
      ki = (0 - ki);
      kd = (0 - kd);
	}   
  controllerDirection = Direction;
}

/* Status Funcions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display 
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
double PID::GetKp(){ return  dispKp; }
double PID::GetKi(){ return  dispKi;}
double PID::GetKd(){ return  dispKd;}
int PID::GetMode(){ return  inAuto ? AUTOMATIC : MANUAL;}
int PID::GetDirection(){ return controllerDirection;}

