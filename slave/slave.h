#ifndef _SLAVE_H
#define _SLAVE_H

#include "Transmitter.h"

#include <QObject>


class dummyObj : public QObject
{
  Q_OBJECT;

  public:
	dummyObj(Transmitter *);

  public slots:
    void sendCPULoad(void);

 private:
	Transmitter *transmitter;
};

#endif
