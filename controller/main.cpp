#include <QApplication>
#include <QtGui>

#include "Transmitter.h"
#include "main.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  dummyObj *dummy = new dummyObj;
  Transmitter transmitter("192.168.3.3", 12347);
  QObject::connect(&transmitter, SIGNAL(rtt(int)), dummy, SLOT(printRtt(int)));
  QObject::connect(&transmitter, SIGNAL(uptime(int)), dummy, SLOT(printUptime(int)));
  QObject::connect(&transmitter, SIGNAL(loadAvg(float)), dummy, SLOT(printLoadAvg(float)));
  QObject::connect(&transmitter, SIGNAL(wlan(int)), dummy, SLOT(printWlan(int)));

  transmitter.initSocket();

  QWidget window;
  window.setWindowTitle("Controller");

  QPushButton *button = new QPushButton("Send ping");
  QObject::connect(button, SIGNAL(clicked()), &transmitter, SLOT(sendPing()));

  QHBoxLayout *layout = new QHBoxLayout();
  layout->addWidget(button);
  window.setLayout(layout);

  window.show();

  return app.exec();
}



void dummyObj::printRtt(int ms)
{
  qDebug() << "RTT:" << ms;
}



void dummyObj::printUptime(int seconds)
{
  qDebug() << "uptime:" << seconds << "seconds";
}



void dummyObj::printLoadAvg(float avg)
{
  qDebug() << "Load avg:" << avg;
}



void dummyObj::printWlan(int percent)
{
  qDebug() << "WLAN:" << percent;
}

