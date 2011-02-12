QMAKE_CXXFLAGS += -funsigned-char

CONFIG += qt debug warn_on link_prl
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += main.cpp
SOURCES += Controller.cpp
SOURCES += VideoReceiver.cpp

HEADERS += Controller.h
HEADERS += VideoReceiver.h

TARGET = controller


unix:!symbian {
    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-0.10 gstreamer-app-0.10 gstreamer-interfaces-0.10
}
