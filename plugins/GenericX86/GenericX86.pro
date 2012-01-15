QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

INCLUDEPATH += ../../common
LIBS += -L../../common -lcommon

SOURCES += GenericX86.cpp

HEADERS += GenericX86.h
HEADERS += ../../common/Hardware.h

TARGET = generic_x86
