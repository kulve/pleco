QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

SOURCES += GenericX86.cpp
SOURCES += GenericX86Factory.cpp

HEADERS += GenericX86.h
HEADERS += GenericX86Factory.h
HEADERS += ../../common/Hardware.h
HEADERS += ../../common/HardwareFactory.h

TARGET = generic_x86
