#ifndef _GUMSTIX_OVERO_FACTORY_H
#define _GUMSTIX_OVERO_FACTORY_H

#include "../../common/HardwareFactory.h"
#include "GumstixOvero.h""

#include <QObject>
#include <QString>



Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOveroFactory)



class GumstixOveroFactory: public QObject, public HardwareFactory
{

  Q_OBJECT;
  Q_INTERFACES(HardwareFactory);

 public:
  Hardware *newHardware(void) {
	new GumstixOvero();
  }
};

#endif
