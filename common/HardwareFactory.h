#ifndef _HARDWARE_FACTORY_H
#define _HARDWARE_FACTORY_H

#include "Hardware.h"

#include <QString>
#include <QtPlugin>

class HardwareFactory
{

 public:
  virtual ~HardwareFactory() {}
  virtual Hardware *newHardware(void) = 0;
};

Q_DECLARE_INTERFACE(HardwareFactory, "HardwareFactory");

#endif
