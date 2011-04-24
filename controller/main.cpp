

#include "Controller.h"

int main(int argc, char *argv[])
{

  Controller controller(argc, argv);

  QStringList args = QCoreApplication::arguments();

  if (args.contains("--help")
   || args.contains("-h")) {
    printf("Usage: %s [ip of relay server]\n",
      qPrintable(QFileInfo(argv[0]).baseName()));
    return 0;
  }

  controller.createGUI();

  QString relay = "127.0.0.1";
  if (args.length() > 1) {
	relay = args.at(1);
  }

  controller.connect(relay, 12347);

  return controller.exec();
}
