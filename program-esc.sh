#!/bin/sh

ALLPWMS="8 9 10 11"
PWMS="8"

# Verify that we have at least one PWM device
if [ ! -e /dev/pwm8 ]
then
  echo "No pwm devices found!"
  exit 1
fi


echo "Shutting down the PWM signal"

# Turn PWM signal completely off
for pwm in $ALLPWMS
do
  echo "0" > /dev/pwm$pwm
done

sleep 1

echo "Setting the duty cycle to 100%"

# Turn ESCs to 100%
for pwm in $PWMS
do
  echo "20000" > /dev/pwm$pwm
done

# Wait 3 sec for 50Hz, 1 sec for 200Hz seconds for the beep
sleep 1

echo "Setting the duty cycle to 0%"

# Turn ESCs to 0%
for pwm in $PWMS
do
  echo "10000" > /dev/pwm$pwm
done
