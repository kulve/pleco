QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

CONFIG += qt debug warn_on link_prl link_pkgconfig
QT += network
PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10
for(PKG, $$list($$unique(PKGCONFIG))) {
    !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
}

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += Slave.cpp
SOURCES += VideoSender.cpp
SOURCES += ControlBoard.cpp
SOURCES += main.cpp

HEADERS += Slave.h
HEADERS += VideoSender.h
HEADERS += ControlBoard.h
HEADERS += ../common/Hardware.h

TARGET = slave
