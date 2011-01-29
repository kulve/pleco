#include <QCoreApplication>

#include "Transmitter.h"
#include "Slave.h"

int main(int argc, char *argv[])
{
  Slave slave(argc, argv);

  slave.connect("192.168.3.3", 8500);

  return slave.exec();
}
