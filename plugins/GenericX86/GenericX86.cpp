#include "GenericX86.h"

#include <QtPlugin>



Q_EXPORT_PLUGIN2(generic_x86, GenericX86)



GenericX86::GenericX86(void)
{

}



bool GenericX86::init(void)
{

  return true;
}



QString GenericX86::getVideoEncoderName(void) const
{
  return "ffenc_h263";
}
