QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror
QT += network

TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

INCLUDEPATH += ../../common
INCLUDEPATH += ../../slave
LIBS += -L../../common -lcommon

SOURCES += GumstixOvero.cpp

HEADERS += GumstixOvero.h
HEADERS += ../../common/Hardware.h

TARGET = gumstix_overo
