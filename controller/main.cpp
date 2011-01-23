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

  transmitter.initSocket();

  QWidget window;
  window.setWindowTitle("Controller");

  QPushButton *button = new QPushButton("Send ping");
  QObject::connect(button, SIGNAL(clicked()), &transmitter, SLOT(ping()));

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
