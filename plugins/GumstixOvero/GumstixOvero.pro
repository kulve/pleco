QMAKE_CXXFLAGS += -funsigned-char
QMAKE_CXXFLAGS += -Werror

TEMPLATE = lib

CONFIG += qt debug warn_on create_prl plugin

SOURCES += GumstixOvero.cpp

HEADERS += GumstixOvero.h
HEADERS += GumstixOveroFactory.h
HEADERS += ../../common/Hardware.h
HEADERS += ../../common/HardwareFactory.h

TARGET = gumstix_overo
