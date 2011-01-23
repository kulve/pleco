CONFIG += qt debug warn_on
QT += network

HEADERS += Transmitter.h
HEADERS += msg.h
SOURCES += main.cpp
SOURCES += Transmitter.cpp
SOURCES += msg.c

TARGET = qt-controller
