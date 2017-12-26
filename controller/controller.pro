QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

CONFIG += qt debug warn_on link_prl link_pkgconfig
QT += network widgets
PKGCONFIG += gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 libdrm
for(PKG, $$list($$unique(PKGCONFIG))) {
     !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
}


INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp
SOURCES += VideoReceiver.cpp
SOURCES += AudioReceiver.cpp
SOURCES += Joystick.cpp

HEADERS += Controller.h
HEADERS += VideoReceiver.h
HEADERS += AudioReceiver.h
HEADERS += Joystick.h

TARGET = controller

INSTALLS += target
target.path = $$PREFIX/bin
