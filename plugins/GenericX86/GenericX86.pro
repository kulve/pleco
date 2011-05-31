QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror


TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

INCLUDEPATH += ../../common
INCLUDEPATH += ../../slave
LIBS += -L../../common -lcommon

SOURCES += GenericX86.cpp

HEADERS += GenericX86.h
HEADERS += ../../common/Hardware.h
HEADERS += ../../common/IMU.h

TARGET = generic_x86
