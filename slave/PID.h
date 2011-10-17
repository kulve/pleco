#ifndef _PID_H
#define _PID_H

class PID
{


 public:

  //Constants used in some of the functions below
#define AUTOMATIC	1
#define MANUAL	0
#define DIRECT  0
#define REVERSE  1
#define ADAPTIVE_TUNING_LOW_THRESHOLD     20
#define ADAPTIVE_TUNING_HIGH_THRESHOLD    30

#define AGGRESSIVE_TUNING_KP               4
#define AGGRESSIVE_TUNING_KI             0.2
#define AGGRESSIVE_TUNING_KD               1
#if 0
#define CONSERVATIVE_TUNING_KP             1
#define CONSERVATIVE_TUNING_KI          0.05
#define CONSERVATIVE_TUNING_KD          0.25
#else
#define CONSERVATIVE_TUNING_KP          0.10
#define CONSERVATIVE_TUNING_KI          0.02
#define CONSERVATIVE_TUNING_KD          0.05
#endif


#define HISTORY_LEN                        5

  //commonly used functions **************************************************************************
  PID();                                // * constructor.
	
  void SetMode(int Mode);               // * sets PID to either Manual (0) or Auto (non-0)

  double Compute();                       // * performs the PID calculation.  it should be
  //   called every time loop() cycles. ON/OFF and
  //   calculation frequency can be set using SetMode
  //   SetSampleTime respectively

  void SetOutputLimits(double, double); //clamps the output to a specific range. 0-255 by default, but
  //it's likely the user will want to change this depending on
  //the application
	


  //available but not commonly used functions ********************************************************
  void SetTunings(double, double,       // * While most users will set the tunings once in the 
				  double);         	  //   constructor, this function gives the user the option
  //   of changing tunings during runtime for Adaptive control
  void SetControllerDirection(int);	  // * Sets the Direction, or "Action" of the controller. DIRECT
  //   means the output will increase when error is positive. REVERSE
  //   means the opposite.  it's very unlikely that this will be needed
  //   once it is set in the constructor.
  void SetSampleTime(int);              // * sets the frequency, in Milliseconds, with which 
  //   the PID calculation is performed.  default is 100
										  
										  
										  
  //Display functions ****************************************************************
  double GetKp();						  // These functions query the pid for interal values.
  double GetKi();						  //  they were created mainly for the pid front-end,
  double GetKd();						  // where it's important to know what is actually 
  int GetMode();						  //  inside the PID.
  int GetDirection();					  //
  void SetTarget(double target);
  void SetActual(double actual);

 private:
  void AdaptTunings(double error = 1);
  void Initialize();

  // Use this when the compute is called
  double target;
  double actual;
  double output;

  double dispKp;				// * we'll hold on to the tuning parameters in user-entered 
  double dispKi;				//   format for display purposes
  double dispKd;				//
    
  double kp;                  // * (P)roportional Tuning Parameter
  double ki;                  // * (I)ntegral Tuning Parameter
  double kd;                  // * (D)erivative Tuning Parameter

  int controllerDirection;

  unsigned long lastTime;
  double ITerm, lastInput, lastOutput;
  double pastInputs[HISTORY_LEN];
  double pastTimes[HISTORY_LEN];
  int pastIndex;
  bool historyFull;

  int SampleTime;
  double outMin, outMax;
  bool inAuto;
};
#endif

