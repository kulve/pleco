#include "GumstixOvero.h"

#include <QtPlugin>



Q_EXPORT_PLUGIN2(gumstix_overo, GumstixOvero)



GumstixOvero::GumstixOvero(void)
{

}



bool GumstixOvero::init(void)
{

  return true;
}



QString GumstixOvero::getVideoEncoderName(void) const
{
  return "dsph263enc";
}

