CONFIG += qt debug warn_on
QT += network

INCLUDEPATH += ../common
LIBS += -L../common -lcommon

SOURCES += slave.cpp

HEADERS += slave.h

TARGET = slave
