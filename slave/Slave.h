#ifndef _SLAVE_H
#define _SLAVE_H

#include "Transmitter.h"

#include <QCoreApplication>

class Slave : public QCoreApplication
{
  Q_OBJECT;

 public:
  Slave(int &argc, char **argv);
  ~Slave();
  void connect(QString host, quint16 port);

 private slots:
  void readStats(void);
  void processError(QProcess::ProcessError error);
  void processStarted(void);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

 private:
	Transmitter *transmitter;
	QProcess *process;
	QList<int> *stats;

	void sendStats(void);
};

#endif
