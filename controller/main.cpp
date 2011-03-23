

#include "Controller.h"

int main(int argc, char *argv[])
{

  Controller controller(argc, argv);

  controller.createGUI();

  QStringList args = QCoreApplication::arguments();

  QString relay = "192.168.3.3";
  if (args.length() > 1) {
	relay = args.at(1);
  }

  controller.connect(relay, 12347);

  return controller.exec();
}
