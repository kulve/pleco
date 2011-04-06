QMAKE_CXXFLAGS += -funsigned-char

CONFIG += qt debug warn_on link_prl
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += Slave.cpp
SOURCES += Motor.cpp
SOURCES += VideoSender.cpp
SOURCES += main.cpp

HEADERS += Slave.h
HEADERS += Motor.h
HEADERS += VideoSender.h

TARGET = slave

unix:!symbian {
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10
    for(PKG, $$list($$unique(PKGCONFIG))) {
      !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
    }
}
