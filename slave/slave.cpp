#include <QApplication>
#include <QtGui>

#include "Transmitter.h"
#include "slave.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  Transmitter transmitter("192.168.3.3", 8500);
  dummyObj *dummy = new dummyObj(&transmitter);

  transmitter.initSocket();

  QWidget window;
  window.setWindowTitle("Slave");

  QPushButton *button = new QPushButton("Send ping");
  QObject::connect(button, SIGNAL(clicked()), &transmitter, SLOT(ping()));

  QHBoxLayout *layout = new QHBoxLayout();
  layout->addWidget(button);
  window.setLayout(layout);

  window.show();

  QTimer *cpuTimer = new QTimer();
  QObject::connect(cpuTimer, SIGNAL(timeout()), dummy, SLOT(sendCPULoad()));
  cpuTimer->start(1000);

  return app.exec();
}



dummyObj::dummyObj(Transmitter *transmitter):
  transmitter(transmitter)
{
  // Nothing to do here
}



void dummyObj::sendCPULoad(void)
{
  int percent = 5;

  qDebug() << "Sending CPU load:" << percent;

  transmitter->sendCPULoad(percent);
}
