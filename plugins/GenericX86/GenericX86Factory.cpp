#include "GenericX86Factory.h"
#include "GenericX86.h"
#include "../../common/Hardware.h"

#include <QObject>
#include <QString>
#include <QtPlugin>



Hardware *GenericX86Factory::newHardware(void) {
  return new GenericX86();
}



Q_EXPORT_PLUGIN2(generic_x86, GenericX86Factory)
