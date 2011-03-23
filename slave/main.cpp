#include <QCoreApplication>
#include <QStringList>

#include "Transmitter.h"
#include "Slave.h"

int main(int argc, char *argv[])
{
  Slave slave(argc, argv);

  if (!slave.init()) {
	qFatal("Failed to init slave class. Exiting..");
	return 1;
  }

  QStringList args = QCoreApplication::arguments();

  QString relay = "192.168.3.3";
  if (args.length() > 1) {
	relay = args.at(1);
  }

  slave.connect(relay, 8500);

  return slave.exec();
}
