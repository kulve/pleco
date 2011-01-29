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

 public slots:
  void sendCPULoad(void);
  
 private:
	Transmitter *transmitter;
};

#endif
