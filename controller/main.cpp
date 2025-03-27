/*
 * Copyright 2017 Tuomas Kulve, <tuomas@kulve.fi>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include "Controller.h"
#include "Controller-qt.h"

int main(int argc, char *argv[])
{
  #ifdef ENABLE_OPENHMD
  Controller_gl controller(argc, argv);
  #else
  Controller_qt controller(argc, argv);
  endif

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

/* Emacs indentatation information
   Local Variables:
   indent-tabs-mode:nil
   tab-width:2
   c-basic-offset:2
   End:
*/
