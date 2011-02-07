QMAKE_CXXFLAGS += -funsigned-char

TEMPLATE = lib

CONFIG += qt debug warn_on staticlib create_prl
QT += network

SOURCES += Transmitter.cpp
SOURCES += Message.cpp

HEADERS += Transmitter.h
HEADERS += Message.h
