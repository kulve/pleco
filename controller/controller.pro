QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

CONFIG += qt debug warn_on link_prl
QT += network
QT += opengl

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp
SOURCES += VideoReceiver.cpp
SOURCES += Plotter.cpp
SOURCES += GLWidget.cpp
SOURCES += qtlogo.cpp

HEADERS += Controller.h
HEADERS += VideoReceiver.h
HEADERS += Plotter.h
HEADERS += GLWidget.h
HEADERS += qtlogo.h

TARGET = controller


unix:!symbian {
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10 gstreamer-interfaces-0.10
    for(PKG, $$list($$unique(PKGCONFIG))) {
      !system(pkg-config --exists $$PKG):error($$PKG development files are missing)
    }
}
