#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <QObject>
#include <QString>

class Hardware
{
 public:
  virtual ~Hardware() {}
  
  virtual bool init(void) = 0;
  virtual QString getVideoEncoderName(void) const = 0;
};

Q_DECLARE_INTERFACE(Hardware, "org.pleco.Hardware")

#endif
