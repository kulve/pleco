QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

TEMPLATE = lib

CONFIG += qt debug warn_on staticlib create_prl
QT += network

INCLUDEPATH += ../slave

SOURCES += Transmitter.cpp
SOURCES += Message.cpp

HEADERS += Transmitter.h
HEADERS += Message.h
HEADERS += Hardware.h
