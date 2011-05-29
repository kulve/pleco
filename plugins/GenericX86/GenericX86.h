#ifndef _GENERIC_X86_H
#define _GENERIC_X86_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "../../common/Hardware.h"

class GenericX86 : public Hardware
{
  Q_OBJECT;

 public:
  GenericX86(void);
  ~GenericX86(void);
  bool init(void);
  QString getVideoEncoderName(void) const;

  bool initIMU(void);
  bool enableIMU(bool enable);

 private slots:
  void generateData(void);

 private:
  QString encoderName;
  QTimer *dataTimer;
};

#endif
