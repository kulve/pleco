#ifndef _SLAVE_H
#define _SLAVE_H

#include "Transmitter.h"
#include "Motor.h"

#include <QCoreApplication>

class Slave : public QCoreApplication
{
  Q_OBJECT;

 public:
  Slave(int &argc, char **argv);
  ~Slave();
  bool init(void);
  void connect(QString host, quint16 port);

 private slots:
  void readStats(void);
  void processError(QProcess::ProcessError error);
  void processStarted(void);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void updateCameraX(int degrees);
  void updateCameraY(int degrees);
  void updateMotorRight(int percent);
  void updateMotorLeft(int percent);

 private:
	Transmitter *transmitter;
	QProcess *process;
	QList<int> *stats;
	Motor *motor;

	void sendStats(void);
};

#endif
