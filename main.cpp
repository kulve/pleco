#include <QApplication>
#include <QtGui>

#include "Transmitter.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  Transmitter transmitter("192.168.3.3", 12347);

  transmitter.initSocket();

  QWidget window;
  window.setWindowTitle("Window layout");

  QPushButton *button = new QPushButton("Send ping");
  QObject::connect(button, SIGNAL(clicked()), &transmitter, SLOT(ping()));

  QHBoxLayout *layout = new QHBoxLayout();
  layout->addWidget(button);
  window.setLayout(layout);

  window.show();

  return app.exec();
}
