QMAKE_CXXFLAGS += -funsigned-char

CONFIG += qt debug warn_on link_prl
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += Slave.cpp
SOURCES += Motor.cpp
SOURCES += main.cpp

HEADERS += Slave.h
HEADERS += Motor.h

TARGET = slave
