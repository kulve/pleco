QMAKE_CXXFLAGS += -funsigned-char -fexceptions -fstack-protector -fno-omit-frame-pointer --param=ssp-buffer-size=4 -fmessage-length=0

TEMPLATE = lib

CONFIG += qt debug warn_on staticlib create_prl
QT += network

SOURCES += Transmitter.cpp
SOURCES += Message.cpp

HEADERS += Transmitter.h
HEADERS += Message.h
