QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

CONFIG += qt debug warn_on link_prl link_pkgconfig
QT += network opengl
PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10 gstreamer-interfaces-0.10
for(PKG, $$list($$unique(PKGCONFIG))) {
     !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
}


INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp
SOURCES += VideoReceiver.cpp

HEADERS += Controller.h
HEADERS += VideoReceiver.h

TARGET = controller
