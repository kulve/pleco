/*
 * Copyright 2012 Tuomas Kulve, <tuomas.kulve@snowcap.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ControlBoard.h"

// For traditional serial port handling
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>          // errno
#include <string.h>         // strerror

// How many characters to read from the Control Board
#define CB_BUFFER_SIZE   1

/*
 * Constructor for the ControlBoard
 */
ControlBoard::ControlBoard(QString serialDevice):
  serialFD(-1), serialDevice(serialDevice), serialPort(), serialData(),
  enabled(false)

{
  QObject::connect(&serialPort, SIGNAL(readyRead()),
                   this, SLOT(readPendingSerialData()));

}



/*
 * Destructor for the ControlBoard
 */
ControlBoard::~ControlBoard()
{
  // Close serial port
  if (serialFD >= 0) {
    serialPort.close();
    close(serialFD);
	serialFD = -1;
  }
}



/*
 * Init ControlBoard. Returns false on failure
 */
bool ControlBoard::init(void)
{

  // Enable IMU
  if (!openSerialDevice()) {
    qCritical("Failed to open and setup serial port");
    return false;
  }

  enabled = true;
  return true;
}



/*
 * Read data from the serial device
 */
void ControlBoard::readPendingSerialData(void)
{
  while (serialPort.bytesAvailable() > 0) {

    // FIXME: try to avoid unneccessary mallocing
    serialData.append(serialPort.readAll());

    //qDebug() << "in" << __FUNCTION__ << ", data size: " << serialData.size();

  }

  parseSerialData();
}



/*
 * Open a serial device and pass the file descriptor to tcpsocket to
 * get readyRead() signal.
 */
bool ControlBoard::openSerialDevice(void)
{
  // QFile doesn't support reading UNIX device nodes using readyRead()
  // signal, so we trick around that using TCP socket class.  We'll
  // set up the file descriptor without Qt and then pass the properly
  // set up file descriptor to QTcpSocket for handling the incoming
  // data.

  // Open device
  serialFD = open(serialDevice.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serialFD < 0) {
    qCritical("Failed to open IMU device: %s", strerror(errno));
    return false;
  }

   struct termios newtio;

 // clear struct for new port settings
  bzero(&newtio, sizeof(newtio));

  // control mode flags
  newtio.c_cflag = CS8 | CLOCAL | CREAD;

  // input mode flags
  newtio.c_iflag = 0;

  // output mode flags
  newtio.c_oflag = 0;

  // local mode flags
  newtio.c_lflag = 0;

  // set input/output speeds
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);

  // now clean the serial line and activate the settings
  tcflush(serialFD, TCIFLUSH);
  tcsetattr(serialFD, TCSANOW, &newtio);

  // Set the file descriptor for our TCP socket class
  serialPort.setSocketDescriptor(serialFD);
  serialPort.setReadBufferSize(CB_BUFFER_SIZE);

  return true;
}



/*
 * Parse the incoming data for valid messages
 */
void ControlBoard::parseSerialData(void)
{
  if (serialData.size() < CB_BUFFER_SIZE) {
	// Not enough data
	return;
  }

  // Check for a full message ending to \n
  if (serialData.at(serialData.size() - 1) == '\n') {
	qDebug() << __FUNCTION__ << "have msg:" << serialData.data();
  } else {
	// Wait for more data
	return;
  }

  // Parse temperature
  if (serialData.startsWith("t: ")) {
	serialData.remove(0,3);
	int adc = serialData.trimmed().toInt();

	// Temperature (in 'C) = {(VSENSE – V25) / Avg_Slope} + 25
	// V25 = 0.76V for F2
	// Avg_Slope = 2.5 mV/'C for F2

	float vsense = (3.3/((float)4095)) * adc;
	double temp = (vsense-0.76)/0.0025 + 25;
	qDebug() << __FUNCTION__ << "Temperature:" << temp;

  }

  serialData.clear();
}



/*
 * Set the frequency of all PWMs
 */
void ControlBoard::setPWMFreq(quint32 freq)
{
  if (!enabled) {
	qWarning("%s: Not enabled", __FUNCTION__);
	return;
  }

  QString cmd = "pf" + QString::number(freq) + '\r' + '\n';
  serialPort.write(cmd.toAscii());
}


/*
 * Stop the PWM signal of the specified PWM.
 */
void ControlBoard::stopPWM(quint8 pwm)
{
  if (!enabled) {
	qWarning("%s: Not enabled", __FUNCTION__);
	return;
  }

  QString cmd = "ps" + QString::number(pwm) + '\r' + '\n';
  serialPort.write(cmd.toAscii());
}


/*
 * Set the duty cycle (0-10000) of the selected PWM.
 */
void ControlBoard::setPWMDuty(quint8 pwm, quint16 duty)
{
  if (!enabled) {
	qWarning("%s: Not enabled", __FUNCTION__);
	return;
  }

  if (duty > 10000) {
	qWarning("%s: Duty out of range: %d", __FUNCTION__, duty);
	return;
  }

  QString cmd = "p" + QString::number(pwm) + QString::number(duty) + '\r' + '\n';
  serialPort.write(cmd.toAscii());
}



// FIXME: this uses now control board's led interface. Should be more generic gpio interface
void ControlBoard::setGPIO(quint16 gpio, quint16 enable)
{
  QString cmd = "l";

  (void)gpio;

  if (enable) {
	cmd += "1";
  } else {
	cmd += "0";
  }

  cmd += "\n";

  serialPort.write(cmd.toAscii());
}

