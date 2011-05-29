#ifndef _GUMSTIX_OVERO_H
#define _GUMSTIX_OVERO_H

#include <QObject>
#include <QString>

#include "../../common/Hardware.h"

class GumstixOvero : public Hardware
{
  Q_OBJECT;

 public:
  GumstixOvero(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

  bool initIMU(void);
  bool enableIMU(bool);

 signals:
  void IMURaw(int accuracy_bytes, int data[9]);

};

#endif
