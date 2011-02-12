

#include "Controller.h"

int main(int argc, char *argv[])
{

  Controller controller(argc, argv);

  controller.createGUI();
  controller.connect("192.168.3.3", 12347);

  return controller.exec();
}
