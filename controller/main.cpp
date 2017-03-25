

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
  QByteArray env_relay = qgetenv("PLECO_RELAY_IP");
  if (args.length() > 1) {
	relay = args.at(1);
  } else if (!env_relay.isNull()) {
	relay = env_relay;
  }

  QHostInfo info = QHostInfo::fromName(relay);

  if (info.addresses().isEmpty()) {
	qWarning() << "Failed to get IP for" << relay;
	return 0;
  }

  QHostAddress address = info.addresses().first();

  controller.connect(address.toString(), 12347);

  return controller.exec();
}
