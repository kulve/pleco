#ifndef _GENERICX86_H
#define _GENERICX86_H

#include <QObject>
#include <QString>

#include "../../common/Hardware.h"

class GenericX86 : public QObject, public Hardware
{
  Q_OBJECT;

  Q_INTERFACES(Hardware);
	
 public:
  GenericX86(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

 private:
  QString encoderName;
};

#endif
