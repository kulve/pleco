QMAKE_CXXFLAGS += -funsigned-char

TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

SOURCES += GenericX86.cpp

HEADERS += GenericX86.h

TARGET = generic_x86
