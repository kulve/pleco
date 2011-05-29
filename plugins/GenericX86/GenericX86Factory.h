#ifndef _GENERIC_X86_FACTORY_H
#define _GENERIC_X86_FACTORY_H

#include "../../common/HardwareFactory.h"
#include "GenericX86.h"

#include <QObject>
#include <QString>

class GenericX86Factory: public QObject, public HardwareFactory
{

  Q_OBJECT;
  Q_INTERFACES(HardwareFactory);

 public:
  Hardware *newHardware(void);
};

#endif
