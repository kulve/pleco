#include "GenericX86.h"

#include <QtPlugin>
#include <QDebug>


Q_EXPORT_PLUGIN2(generic_x86, GenericX86)



GenericX86::GenericX86(void):
  encoderName("ffenc_h263")
{

}



bool GenericX86::init(void)
{

  return true;
}



QString GenericX86::getVideoEncoderName(void) const
{
  qDebug() << __FUNCTION__ << encoderName;
  return encoderName;
}
