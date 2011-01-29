#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include "Transmitter.h"

#include <QApplication>
#include <QtGui>

class Controller : public QApplication
{
  Q_OBJECT;

 public:
  Controller(int &argc, char **argv);
  ~Controller(void);
  void createGUI(void);
  void connect(QString host, quint16 port);

 private slots:
  void updateRtt(int ms);
  void updateUptime(int seconds);
  void updateLoadAvg(float avg);
  void updateWlan(int percent);

 private:
  Transmitter *transmitter;
  QWidget *window;

  QLabel *labelUptime;
  QLabel *labelLoadAvg;
  QLabel *labelWlan;
};

#endif
