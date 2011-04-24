#ifndef _GENERICX86_H
#define _GENERICX86_H

#include <QObject>
#include <QString>

#include "../../common/Hardware.h"

class GumstixOvero : public QObject, public Hardware
{
  Q_OBJECT;

  Q_INTERFACES(Hardware);
	
 public:
  GumstixOvero(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

};

#endif
