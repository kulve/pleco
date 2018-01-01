/*
 * Copyright 2012 Tuomas Kulve, <tuomas@kulve.fi>
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

#ifndef _CONTROLBOARD_H
#define _CONTROLBOARD_H

#include <QString>
#include <QTcpSocket>
#include <QTimer>

#define  CB_PWM1       1
#define  CB_PWM2       2
#define  CB_PWM3       3
#define  CB_PWM4       4
#define  CB_PWM5       5
#define  CB_PWM6       6
#define  CB_PWM7       7
#define  CB_PWM8       8


// Device specific defines
#define  CB_PWM_SPEED                 CB_PWM1
#define  CB_PWM_TURN                  CB_PWM2
#define  CB_PWM_TURN2                 CB_PWM7
#define  CB_PWM_CAMERA_X              CB_PWM3
#define  CB_PWM_CAMERA_Y              CB_PWM4

#define  CB_PWM_SPEED_LEFT            CB_PWM5
#define  CB_PWM_SPEED_RIGHT           CB_PWM6
#define  CB_GPIO_DIRECTION_LEFT       2
#define  CB_GPIO_DIRECTION_RIGHT      3
#define  CB_GPIO_SPEED_ENABLE_LEFT    4
#define  CB_GPIO_SPEED_ENABLE_RIGHT   5

#define  CB_GPIO_LED1                 0 // HACK
#define  CB_GPIO_HEAD_LIGHTS          5
#define  CB_GPIO_REAR_LIGHTS          1

class ControlBoard : public QObject
{
  Q_OBJECT;

 public:
  ControlBoard(QString serialDevice);
  ~ControlBoard();
  bool init(void);
  void setPWMFreq(quint32 freq);
  void setPWMDuty(quint8 pwm, quint16 duty);
  void stopPWM(quint8 pwm);
  void setGPIO(quint16 gpio, quint16 enable);
  void sendPing(void);

  signals:
  void debug(QString *msg);

  void temperature(quint16 value);
  void distance(quint16 value);
  void current(quint16 value);
  void voltage(quint16 value);

  private slots:
    void readPendingSerialData(void);
    void portError(QAbstractSocket::SocketError socketError);
    void portDisconnected(void);
    void reopenSerialDevice(void);

  private:
    void parseSerialData(void);
    bool openSerialDevice(void);
    void closeSerialDevice(void);
    void writeSerialData(QString &msg);

    int serialFD;
    QString serialDevice;
    QTcpSocket serialPort;
    QByteArray serialData;
    bool enabled;
    QTimer reopenTimer;
    QTimer wdgTimer;
};

#endif

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
