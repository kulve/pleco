/*
 * Parts taken from http://docwiki.gumstix.org/Sample_code/C/gpregs
 * OMAP3 specific stuff from:
 * - http://old.nabble.com/PWM-Driver-for-Overo--td22888603.html
 * - OMAP35x TRM (spruf98d.pdf)
 * Register writes from devmem2.c by Jan-Derk Bakker (jdb@lartmaker.nl)
 */

#include "Motor.h"

#include <errno.h>          /* errno */
#include <fcntl.h>          /* open */
#include <unistd.h>         /* close */
#include <sys/mman.h>       /* mmap, munmap */
#include <sys/stat.h>       /* open */


/* TCLR bit positions */
#define TCLR_ST         0
#define TCLR_AR         1
#define TCLR_PTV        2
#define TCLR_PRE        5
#define TCLR_CE         6
#define TCLR_SCPWM      7
#define TCLR_TCM        8
#define TCLR_TRG       10
#define TCLR_PT        12
#define TCLR_CAPT_MODE 13
#define TCLR_GPO_CFG   14


#define CONTROL_PADCONF_UART2_CTS 0x48002174
#define CONTROL_PADCONF_UART2_TX  0x48002178

#define TCLR      0x0024
#define TCRR      0x0028
#define TLDR      0x002C
#define TMAR      0x0038

#define GPTIMER1  0x48318000
#define GPTIMER2  0x49032000
#define GPTIMER3  0x49034000
#define GPTIMER4  0x49036000
#define GPTIMER5  0x49038000
#define GPTIMER6  0x4903A000
#define GPTIMER7  0x4903C000
#define GPTIMER8  0x4903E000
#define GPTIMER9  0x49040000
#define GPTIMER10 0x48086000
#define GPTIMER11 0x48088000
#define GPTIMER12 0x48304000

/* Smallest (quite rounded) value for mmap */
#define MAP_ADDR               0x48000000
/* "biggest - smallest" to cover everything. 4k is reserver for each timer.*/
#define MAP_SIZE               ((GPTIMER9 + 4096) - MAP_ADDR)

#define CAMERA_X_TIMER         GPTIMER8
#define CAMERA_Y_TIMER         GPTIMER9
#define MOTOR_RIGHT_TIMER      GPTIMER10
#define MOTOR_LEFT_TIMER       GPTIMER11



Motor::Motor(void):
  map(NULL),
  motorRightSpeed(0), motorLeftSpeed(0),
  cameraXDegrees(0), cameraYDegrees(0),
  enabled(false)
{
  qDebug() << "in" << __FUNCTION__;
  
}



Motor::~Motor(void)
{
  qDebug() << "in" << __FUNCTION__;
  
  // Center or zero or motors
  motorRight(0);
  motorLeft(0);
  cameraX(0);
  cameraY(0);

  // Shutdown all motors
  motorRightEnable(false);
  motorLeftEnable(false);
  cameraXEnable(false);
  cameraYEnable(false);

  // Unmap /dev/mem
  if (map) {
	if (munmap(map, MAP_SIZE) == -1) {
	  qWarning("WARNING: Failed to munmap /dev/mmem: %s\n",
			   strerror(errno));	
	}
	map = NULL;
  }

  // Close /dev/mem
  if (fd > 0) {
	close(fd);
	fd = -1;
  }

  // Mark as disabled
  enabled = false;
}



bool Motor::init(void)
{

  // FIXME: fail to init if not running on OMAP3

  // Open /dev/mem 
  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
	qWarning("Failed to open /dev/mmem: %s\n", strerror(errno));
	return false;
  }

  // mmap /dev/mmem 
  map = mmap(0, 
			 MAP_SIZE,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED,
			 fd,
			 MAP_ADDR);
  if (map == MAP_FAILED) {
	qWarning("Failed to mmap /dev/mmem: %s\n", strerror(errno));
	close(fd);
	return false;
  }

  qDebug("Mapped 0x%x to %p, (%d bytes)\n", MAP_ADDR, map, MAP_SIZE);

  // set mux GPT8 and GPT11 (mode2)
  setReg(CONTROL_PADCONF_UART2_TX, 0x01020102);

  // set mux GPT9 and GPT10 (mode2)
  setReg(CONTROL_PADCONF_UART2_CTS, 0x01020102);

  enabled = true;

  // Center or zero or motors
  motorRight(0);
  motorLeft(0);
  cameraX(0);
  cameraY(0);

  // Shutdown all motors
  motorRightEnable(false);
  motorLeftEnable(false);
  cameraXEnable(false);
  cameraYEnable(false);

  // Make sure all motors are off
  motorRightEnable(false);
  motorLeftEnable(false);
  cameraXEnable(false);
  cameraYEnable(false);

  return enabled;
}



void Motor::motorRight(int speed)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  // Do nothing if value not changed
  if (motorRightSpeed == speed) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (speed == 0 && motorRightSpeed != 0) {
	motorRightSpeed = speed;
	motorRightEnable(false);
	return;
  }

  // Enable motor if value changed from zero to non-zero
  if (speed != 0 && motorRightSpeed == 0) {
	motorRightEnable(true);
  }

  motorRightSpeed = speed;

  // Scale -100 - +100 -> 0x0 - 0xf

  // No reverse, so anything <0 -> 0
  if (motorRightSpeed < 0) {
	motorRightSpeed = 0;
  }

  // Scale 0 - 100 -> 0x0 - 0xf (~0.16x)
  quint32 value = (quint32)(motorRightSpeed * (double)0.16);

  qDebug() << "Setting right motor speed to:" << motorRightSpeed;

  // Make sure we don't exceed 0xf
  if (value > 0xf) {
	value = 0xf;
  }
  value += 0xfffffff0;

  setReg(MOTOR_RIGHT_TIMER + TMAR, value);
}




void Motor::motorLeft(int speed)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  // Do nothing if value not changed
  if (motorLeftSpeed == speed) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (speed == 0 && motorLeftSpeed != 0) {
	motorLeftSpeed = speed;
	motorLeftEnable(false);
	return;
  }

  // Enable motor if value changed from zero to non-zero
  if (speed != 0 && motorLeftSpeed == 0) {
	motorLeftEnable(true);
  }

  motorLeftSpeed = speed;

  // Scale -100 - +100 -> 0x0 - 0xf

  // No reverse, so anything <0 -> 0
  if (motorLeftSpeed < 0) {
	motorLeftSpeed = 0;
  }

  // Scale 0 - 100 -> 0x0 - 0xf (~0.16x)
  quint32 value = (quint32)(motorLeftSpeed * (double)0.16);

  qDebug() << "Setting right motor speed to:" << motorLeftSpeed;

  // Make sure we don't exceed 0xf
  if (value > 0xf) {
	value = 0xf;
  }
  value += 0xfffffff0;

  setReg(MOTOR_LEFT_TIMER + TMAR, value);
}




void Motor::cameraX(int degrees)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  // Do nothing if value not changed
  if (cameraXDegrees == degrees) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (degrees == 0 && cameraXDegrees != 0) {
	cameraXDegrees = degrees;
	cameraXEnable(false);
	return;
  }

  // Enable motor if value changed from zero to non-zero
  if (degrees != 0 && cameraXDegrees == 0) {
	cameraXEnable(true);
  }

  cameraXDegrees = degrees;
  
  // Update pulse width for camera X servo timer
  // FIXME: use understandable calculations
  quint32 value = 0xFFFC4500 + 120 * cameraXDegrees;
  setReg(CAMERA_X_TIMER + TMAR, value);
}




void Motor::cameraY(int degrees)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  // Do nothing if value not changed
  if (cameraYDegrees == degrees) {
	return;
  }

  // Disable motor if value changed from non-zero to zero
  if (degrees == 0 && cameraXDegrees != 0) {
	cameraYDegrees = degrees;
	cameraYEnable(false);
	return;
  }

  // Enable motor if value changed from zero to non-zero
  if (degrees != 0 && cameraYDegrees == 0) {
	cameraYEnable(true);
  }

  cameraYDegrees = degrees;
  
  // Update pulse width for camera Y servo timer
  // FIXME: use understandable calculations
  quint32 value = 0xFFFC4500 + 120 * cameraYDegrees;
  setReg(CAMERA_Y_TIMER + TMAR, value);
}




void Motor::motorRightEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  if (enable) {
	powerOnMotor(MOTOR_RIGHT_TIMER);
  } else {
	powerOffMotor(MOTOR_RIGHT_TIMER);
  }
}




void Motor::motorLeftEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  if (enable) {
	powerOnMotor(MOTOR_LEFT_TIMER);
  } else {
	powerOffMotor(MOTOR_LEFT_TIMER);
  }
}




void Motor::cameraXEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  if (enable) {
	powerOnServo(CAMERA_X_TIMER);
  } else {
	powerOffServo(CAMERA_X_TIMER);
  }
}




void Motor::cameraYEnable(bool enable)
{
  qDebug() << "in" << __FUNCTION__;
  
  if (!enabled) {
	return;
  }

  if (enable) {
	powerOnServo(CAMERA_Y_TIMER);
  } else {
	powerOffServo(CAMERA_Y_TIMER);
  }
}


quint32 Motor::getReg(quint32 addr)
{

  // FIXME: this doesn't compile on 64 bit computers
#if __arm__
  quint32 val = 0;
  void *reg_addr;

  reg_addr = (void*)((quint32)map + (addr - MAP_ADDR));
  //printf("get_reg: 0x%x\n", addr - MAP_ADDR);
  val = *(quint32*) reg_addr;

  return val;
#else
  return 0;
#endif
}



void Motor::setReg(quint32 addr, quint32 val)
{

  // FIXME: this doesn't compile on 64 bit computers
#if __arm__
  void *reg_addr;

  reg_addr = (void*)((quint32)map + (addr - MAP_ADDR));
  //printf("set_reg: 0x%08x: 0x%08x\n", addr - MAP_ADDR, val);
  *(quint32*) reg_addr = val;
#endif
}



void Motor::powerOnServo(int timer)
{
  setReg(timer + TCLR, 0); /* stop the timer */
  setReg(timer + TLDR, 0xfffc08F0); /* 20ms period */
  setReg(timer + TMAR, 0xfffC551c); /* 1.5ms pulse width */
  setReg(timer + TCRR, 0xffffffff); /* timercounter, FIXME:?? */

  quint32 value =
	(0x1 << TCLR_PT) +    /* pulse or toggle */
	(0x2 << TCLR_TRG) +   /* trigger output mode */
	(0x1 << TCLR_CE) +    /* compare enable */
	(0x1 << TCLR_AR) +    /* autoreload mode */
	(0x1 << TCLR_ST);     /* start/stop */
  /* 0000 0001 1000 0100 0011 */ 

  setReg(timer + TCLR, value); /* start the timer */
}



void Motor::powerOnMotor(int timer)
{
  setReg(timer + TCLR, 0); /* stop the timer */
  setReg(timer + TLDR, 0xfffffff0); /* 2KHz cycle? */
  setReg(timer + TMAR, 0xfffffff8); /* 50% */
  setReg(timer + TCRR, 0xffffffff); /* timercounter?? */

  quint32 value =
	(0x1 << TCLR_PT) +    /* pulse or toggle */
	(0x2 << TCLR_TRG) +   /* trigger output mode */
	(0x1 << TCLR_CE) +    /* compare enable */
	(0x1 << TCLR_AR) +    /* autoreload mode */
	(0x1 << TCLR_ST);     /* start/stop */
  /* 0000 0001 1000 0100 0011 */ 

  setReg(timer + TCLR, value); /* start the timer */
}



void Motor::powerOffServo(int timer)
{
  setReg(timer + TCLR, 0); /* stop the timer */
}



void Motor::powerOffMotor(int timer)
{
  // Motors are shutdown in the same way as the servos, by stopping the timer
  powerOffServo(timer);
}
